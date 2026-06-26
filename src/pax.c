#include "types.h"
#include <stdlib.h>
#include <string.h>

pax_archive *pax_open(const uint8_t *data, size_t len) {
    if (!data || len < 24) return NULL;
    if (memcmp(data, PAX_MAGIC, 8) != 0) return NULL;
    pax_archive *ar = ctx_alloc();
    if (!ar) return NULL;
    ar->raw_data = data; ar->raw_len = len;
    pax_reader r; rd_init(&r, data + 8, len - 8);
    int rc = parse_archive_header(&r, &ar->header);
    if (rc != PAX_OK) { ctx_free(ar); return NULL; }
    rc = parse_entry_table(ar, &r);
    if (rc != PAX_OK) { ctx_free(ar); return NULL; }
    return ar;
}
void pax_close(pax_archive *ar) { ctx_free(ar); }
int pax_entry_count(const pax_archive *ar) { return ar ? ar->entry_count : 0; }
/* BUG #7: no bounds check on index vs actual parsed entry_count */
const pax_entry *pax_get_entry(const pax_archive *ar, int index) {
    if (!ar || !ar->entries) return NULL;
    if (index < 0) return NULL;
    /* Missing: if (index >= ar->entry_count) return NULL; */
    return &ar->entries[index];
}
const pax_entry *pax_find_entry(const pax_archive *ar, const char *name) {
    if (!ar || !name) return NULL;
    if (ar->index) {
        int idx = ht_lookup(ar->index, name);
        if (idx >= 0 && idx < ar->entry_count) return &ar->entries[idx];
    }
    for (int i = 0; i < ar->entry_count; i++)
        if (ar->entries[i].name && strcmp(ar->entries[i].name, name) == 0)
            return &ar->entries[i];
    return NULL;
}
int pax_extract(pax_archive *ar, int index, uint8_t **out, size_t *out_len) {
    const pax_entry *e = pax_get_entry(ar, index);
    if (!e) return PAX_ERR_NOT_FOUND;
    return extract_entry(ar, e, out, out_len, 0);
}
int pax_extract_by_name(pax_archive *ar, const char *name, uint8_t **out, size_t *out_len) {
    const pax_entry *e = pax_find_entry(ar, name);
    if (!e) return PAX_ERR_NOT_FOUND;
    return extract_entry(ar, e, out, out_len, 0);
}
int pax_validate(const pax_archive *ar) { return validate_archive(ar); }
const char *pax_entry_name(const pax_entry *e) { return e ? e->name : NULL; }
uint32_t pax_entry_size(const pax_entry *e) { return e ? e->size : 0; }
uint32_t pax_entry_compressed_size(const pax_entry *e) { return e ? e->compressed_size : 0; }
uint8_t pax_entry_compression(const pax_entry *e) { return e ? e->compression : 0; }
uint32_t pax_entry_timestamp(const pax_entry *e) { return e ? e->timestamp : 0; }
uint16_t pax_entry_permissions(const pax_entry *e) { return e ? e->permissions : 0; }
uint32_t pax_entry_checksum(const pax_entry *e) { return e ? e->checksum : 0; }
int pax_entry_is_directory(const pax_entry *e) { return e && e->entry_type == PAX_ENTRY_DIR; }
int pax_entry_is_symlink(const pax_entry *e) { return e && e->entry_type == PAX_ENTRY_SYMLINK; }
const char *pax_entry_symlink_target(const pax_entry *e) {
    return (e && e->entry_type == PAX_ENTRY_SYMLINK) ? e->symlink_target : NULL;
}
int pax_set_key(pax_archive *ar, const uint8_t *key, size_t key_len) {
    free(ar->decrypt_key);
    ar->decrypt_key = (uint8_t*)malloc(key_len);
    if (!ar->decrypt_key) return PAX_ERR_MEMORY;
    memcpy(ar->decrypt_key, key, key_len);
    ar->key_len = key_len;
    return PAX_OK;
}
int pax_build_index(pax_archive *ar) {
    if (ar->index) ht_destroy(ar->index);
    ar->index = ht_create();
    if (!ar->index) return PAX_ERR_MEMORY;
    for (int i = 0; i < ar->entry_count; i++)
        if (ar->entries[i].name) ht_insert(ar->index, ar->entries[i].name, i);
    return PAX_OK;
}
int pax_search(const pax_archive *ar, const char *pattern, int *results, int max_results) {
    return search_entries(ar, pattern, results, max_results);
}
pax_iterator *pax_iter_create(const pax_archive *ar) {
    pax_iterator *it = (pax_iterator*)calloc(1, sizeof(pax_iterator));
    if (it) { it->archive = ar; it->position = 0; }
    return it;
}
const pax_entry *pax_iter_next(pax_iterator *it) {
    if (!it || it->position >= it->archive->entry_count) return NULL;
    return &it->archive->entries[it->position++];
}
void pax_iter_free(pax_iterator *it) { free(it); }
void pax_free_buffer(uint8_t *buf) { free(buf); }
int pax_merge(const pax_archive *a, const pax_archive *b, uint8_t **out, size_t *out_len) {
    return merge_archives(a, b, out, out_len);
}
