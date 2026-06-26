#include "types.h"
#include <stdlib.h>
#include <string.h>
pax_builder *pax_builder_create(void) {
    pax_builder *b = (pax_builder*)calloc(1, sizeof(pax_builder));
    if (!b) return NULL;
    b->capacity = 16;
    b->entries = (pax_entry*)calloc(b->capacity, sizeof(pax_entry));
    b->data_blocks = (uint8_t**)calloc(b->capacity, sizeof(uint8_t*));
    if (!b->entries || !b->data_blocks) { free(b->entries); free(b->data_blocks); free(b); return NULL; }
    return b;
}
static int builder_grow(pax_builder *b) {
    int nc = b->capacity * 2;
    pax_entry *ne = (pax_entry*)realloc(b->entries, nc * sizeof(pax_entry));
    uint8_t **nd = (uint8_t**)realloc(b->data_blocks, nc * sizeof(uint8_t*));
    if (!ne || !nd) return PAX_ERR_MEMORY;
    b->entries = ne; b->data_blocks = nd; b->capacity = nc;
    return PAX_OK;
}
int pax_builder_add(pax_builder *b, const char *name, const uint8_t *data,
                    size_t len, uint8_t compression) {
    if (b->count >= b->capacity) { int rc = builder_grow(b); if(rc) return rc; }
    pax_entry *e = &b->entries[b->count];
    memset(e, 0, sizeof(*e));
    e->name = strdup(name);
    if (!e->name) return PAX_ERR_MEMORY;
    e->entry_type = PAX_ENTRY_FILE;
    e->size = (uint32_t)len;
    e->permissions = 0644;
    e->compression = compression;
    uint8_t *copy = NULL;
    if (compression == PAX_COMP_RLE) {
        size_t cl; pax_rle_compress(data, len, &copy, &cl);
        if (copy) { e->compressed_size = (uint32_t)cl; }
        else { copy = (uint8_t*)malloc(len); memcpy(copy,data,len); e->compressed_size=(uint32_t)len; e->compression=PAX_COMP_NONE; }
    } else if (compression == PAX_COMP_LZ) {
        size_t cl; pax_lz_compress(data, len, &copy, &cl);
        if (copy) { e->compressed_size = (uint32_t)cl; }
        else { copy = (uint8_t*)malloc(len); memcpy(copy,data,len); e->compressed_size=(uint32_t)len; e->compression=PAX_COMP_NONE; }
    } else {
        copy = (uint8_t*)malloc(len); if (!copy) return PAX_ERR_MEMORY;
        memcpy(copy, data, len); e->compressed_size = (uint32_t)len;
    }
    b->data_blocks[b->count] = copy;
    b->count++;
    return PAX_OK;
}
int pax_builder_add_dir(pax_builder *b, const char *name) {
    if (b->count >= b->capacity) { int rc = builder_grow(b); if(rc) return rc; }
    pax_entry *e = &b->entries[b->count];
    memset(e, 0, sizeof(*e));
    e->name = strdup(name); e->entry_type = PAX_ENTRY_DIR; e->permissions = 0755;
    b->data_blocks[b->count] = NULL;
    b->count++;
    return PAX_OK;
}
int pax_builder_add_symlink(pax_builder *b, const char *name, const char *target) {
    if (b->count >= b->capacity) { int rc = builder_grow(b); if(rc) return rc; }
    pax_entry *e = &b->entries[b->count];
    memset(e, 0, sizeof(*e));
    e->name = strdup(name); e->entry_type = PAX_ENTRY_SYMLINK;
    e->symlink_target = strdup(target); e->permissions = 0777;
    b->data_blocks[b->count] = NULL;
    b->count++;
    return PAX_OK;
}
int pax_builder_finish(pax_builder *b, uint8_t **out, size_t *out_len) {
    pax_buffer buf; buf_init(&buf);
    /* Magic */ buf_append(&buf, PAX_MAGIC, 8);
    /* Header placeholder */ size_t hdr_pos = buf.len;
    buf_append_u16(&buf, 1); buf_append_u16(&buf, 0);
    buf_append_u32(&buf, (uint32_t)b->count);
    uint32_t data_off_placeholder = 0;
    buf_append_u32(&buf, data_off_placeholder);
    buf_append_u32(&buf, 0); /* checksum placeholder */
    /* Entry table */
    for (int i = 0; i < b->count; i++) {
        pax_entry *e = &b->entries[i];
        uint16_t nlen = (uint16_t)strlen(e->name);
        buf_append_u16(&buf, nlen);
        buf_append(&buf, (uint8_t*)e->name, nlen);
        buf_append_u32(&buf, e->size);
        buf_append_u32(&buf, e->compressed_size);
        buf_append_u32(&buf, 0); /* offset filled below */
        buf_append_u8(&buf, e->compression);
        buf_append_u8(&buf, e->entry_type);
        e->checksum = b->data_blocks[i] ? pax_crc32(b->data_blocks[i], e->compressed_size) : 0;
        buf_append_u32(&buf, e->checksum);
        buf_append_u32(&buf, e->timestamp);
        buf_append_u16(&buf, e->permissions);
        if (e->entry_type == PAX_ENTRY_SYMLINK && e->symlink_target) {
            uint16_t tl = (uint16_t)strlen(e->symlink_target);
            buf_append_u16(&buf, tl);
            buf_append(&buf, (uint8_t*)e->symlink_target, tl);
        }
        buf_append_u16(&buf, 0); /* no metadata */
    }
    /* Fix data offset */
    uint32_t doff = (uint32_t)buf.len;
    buf.data[hdr_pos+4] = (uint8_t)(doff>>24); buf.data[hdr_pos+5] = (uint8_t)(doff>>16);
    buf.data[hdr_pos+6] = (uint8_t)(doff>>8); buf.data[hdr_pos+7] = (uint8_t)doff;
    /* Data blocks */
    for (int i = 0; i < b->count; i++) {
        if (b->data_blocks[i] && b->entries[i].compressed_size > 0)
            buf_append(&buf, b->data_blocks[i], b->entries[i].compressed_size);
    }
    *out = buf_detach(&buf, out_len);
    return PAX_OK;
}
void pax_builder_free(pax_builder *b) {
    if (!b) return;
    for (int i = 0; i < b->count; i++) {
        free(b->entries[i].name); free(b->entries[i].symlink_target);
        free(b->data_blocks[i]);
    }
    free(b->entries); free(b->data_blocks); free(b);
}
