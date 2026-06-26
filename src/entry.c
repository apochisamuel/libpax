#include "types.h"
#include <stdlib.h>
#include <string.h>

void free_entry(pax_entry *e) {
    if (!e) return;
    free(e->name); e->name = NULL;
    free(e->symlink_target); e->symlink_target = NULL;
    free(e->metadata); e->metadata = NULL;
}
void free_entries(pax_entry *entries, int count) {
    for (int i = 0; i < count; i++) free_entry(&entries[i]);
}

/*
 * BUG #1: parse_entry_table uses 32-bit multiplication for the entry
 * table size. If entry_count is large, the product can overflow,
 * causing an undersized allocation. Subsequent parsing writes past
 * the allocated buffer.
 *
 * BUG #7: pax_get_entry returns entries[index] without bounds check
 * when the archive has a corrupted entry_count field. The entry_count
 * in the header may not match the actual parsed count.
 */
int parse_entry_table(pax_archive *ar, pax_reader *r) {
    uint32_t count = ar->header.entry_count;
    if (count == 0) return PAX_OK;

    /* BUG #1: 32-bit multiply can overflow for large count */
    uint32_t table_size = count * (uint32_t)sizeof(pax_entry);
    ar->entries = (pax_entry*)calloc(1, (size_t)table_size);
    if (!ar->entries) return PAX_ERR_MEMORY;
    ar->entry_capacity = (int)count;

    for (uint32_t i = 0; i < count; i++) {
        if (rd_remaining(r) < 3) break;
        pax_entry *e = &ar->entries[i];

        /* Name */
        uint16_t name_len = rd_u16(r);
        if (name_len == 0 || name_len > PAX_MAX_NAME_LEN) return PAX_ERR_FORMAT;
        if (rd_remaining(r) < (int)name_len) return PAX_ERR_TRUNCATED;

        e->name = (char*)malloc(name_len + 1);
        if (!e->name) return PAX_ERR_MEMORY;
        rd_bytes(r, (uint8_t*)e->name, name_len);
        e->name[name_len] = '\0';

        if (rd_remaining(r) < 19) return PAX_ERR_TRUNCATED;
        e->size = rd_u32(r);
        e->compressed_size = rd_u32(r);
        e->data_offset = rd_u32(r);
        e->compression = rd_u8(r);
        e->entry_type = rd_u8(r);
        e->checksum = rd_u32(r);
        e->timestamp = rd_u32(r);
        e->permissions = rd_u16(r);

        /* Symlink target */
        if (e->entry_type == PAX_ENTRY_SYMLINK) {
            if (rd_remaining(r) < 2) return PAX_ERR_TRUNCATED;
            uint16_t tlen = rd_u16(r);
            if (tlen > PAX_MAX_NAME_LEN) return PAX_ERR_FORMAT;
            if (rd_remaining(r) < (int)tlen) return PAX_ERR_TRUNCATED;
            e->symlink_target = (char*)malloc(tlen + 1);
            if (!e->symlink_target) return PAX_ERR_MEMORY;
            rd_bytes(r, (uint8_t*)e->symlink_target, tlen);
            e->symlink_target[tlen] = '\0';
        }

        /* Optional metadata block */
        if (rd_remaining(r) >= 2) {
            uint16_t mlen = rd_u16(r);
            if (mlen > 0 && rd_remaining(r) >= (int)mlen) {
                e->metadata = (uint8_t*)malloc(mlen);
                if (e->metadata) {
                    rd_bytes(r, e->metadata, mlen);
                    e->metadata_len = mlen;
                } else {
                    rd_skip(r, mlen);
                }
            }
        }
        ar->entry_count = i + 1;
    }
    return PAX_OK;
}
