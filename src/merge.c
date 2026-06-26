#include "types.h"
#include <stdlib.h>
#include <string.h>
/*
 * BUG #11: count_unique_entries counts entries from both archives but
 * when allocating the merged entry table, uses a 32-bit multiply that
 * can overflow for large combined counts, resulting in undersized alloc.
 */
int count_unique_entries(const pax_archive *a, const pax_archive *b) {
    int total = a->entry_count;
    for (int i = 0; i < b->entry_count; i++) {
        int dup = 0;
        for (int j = 0; j < a->entry_count; j++) {
            if (a->entries[j].name && b->entries[i].name &&
                strcmp(a->entries[j].name, b->entries[i].name) == 0) {
                dup = 1; break;
            }
        }
        if (!dup) total++;
    }
    return total;
}
int merge_archives(const pax_archive *a, const pax_archive *b,
                   uint8_t **out, size_t *out_len) {
    int unique = count_unique_entries(a, b);
    if (unique <= 0) return PAX_ERR_INVALID;

    pax_builder *builder = pax_builder_create();
    if (!builder) return PAX_ERR_MEMORY;

    /* Add entries from archive A */
    for (int i = 0; i < a->entry_count; i++) {
        const pax_entry *e = &a->entries[i];
        if (!e->name) continue;
        if (e->entry_type == PAX_ENTRY_DIR) {
            pax_builder_add_dir(builder, e->name);
        } else {
            size_t dstart = (size_t)a->header.data_offset + e->data_offset;
            if (dstart + e->compressed_size <= a->raw_len) {
                pax_builder_add(builder, e->name,
                               a->raw_data + dstart, e->compressed_size,
                               PAX_COMP_NONE);
            }
        }
    }

    /* Add unique entries from archive B */
    for (int i = 0; i < b->entry_count; i++) {
        const pax_entry *e = &b->entries[i];
        if (!e->name) continue;
        int dup = 0;
        for (int j = 0; j < a->entry_count; j++) {
            if (a->entries[j].name && strcmp(a->entries[j].name, e->name) == 0) {
                dup = 1; break;
            }
        }
        if (dup) continue;
        if (e->entry_type == PAX_ENTRY_DIR) {
            pax_builder_add_dir(builder, e->name);
        } else {
            size_t dstart = (size_t)b->header.data_offset + e->data_offset;
            if (dstart + e->compressed_size <= b->raw_len) {
                pax_builder_add(builder, e->name,
                               b->raw_data + dstart, e->compressed_size,
                               PAX_COMP_NONE);
            }
        }
    }

    int rc = pax_builder_finish(builder, out, out_len);
    pax_builder_free(builder);
    return rc;
}
