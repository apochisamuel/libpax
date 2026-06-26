#include "types.h"
#include <string.h>
int validate_entry(const pax_entry *e, const uint8_t *adata, size_t alen) {
    if (!e->name || strlen(e->name) == 0) return PAX_ERR_INVALID;
    if (e->compression > PAX_COMP_DELTA) return PAX_ERR_FORMAT;
    if (e->entry_type > PAX_ENTRY_SYMLINK) return PAX_ERR_FORMAT;
    if (e->entry_type == PAX_ENTRY_SYMLINK && !e->symlink_target) return PAX_ERR_INVALID;
    return PAX_OK;
}
int verify_entry_checksum(const pax_entry *e, const uint8_t *data) {
    if (!data) return PAX_ERR_INVALID;
    uint32_t computed = pax_crc32(data, e->compressed_size);
    return computed == e->checksum ? PAX_OK : PAX_ERR_CHECKSUM;
}
int validate_archive(const pax_archive *ar) {
    if (!ar) return PAX_ERR_INVALID;
    if (ar->entry_count == 0) return PAX_OK;
    for (int i = 0; i < ar->entry_count; i++) {
        int rc = validate_entry(&ar->entries[i], ar->raw_data, ar->raw_len);
        if (rc != PAX_OK) return rc;
    }
    return PAX_OK;
}
