#include "types.h"
#include <string.h>
#include <stdlib.h>
/*
 * BUG #10: metadata parsing dereferences a key at offset 0
 * when metadata_len > 0 but the first byte is 0 (zero-length key).
 * The strlen on the extracted key reads unintialized stack data,
 * and the subsequent malloc(0)+memcpy causes issues.
 */
int parse_entry_metadata(const uint8_t *data, size_t len,
                         pax_entry *entry) {
    if (!data || len == 0) return PAX_OK;
    pax_reader r;
    rd_init(&r, data, len);
    /* Simple key=value\0 pairs */
    while (rd_remaining(&r) > 0) {
        uint8_t key_len = rd_u8(&r);
        /* BUG #10: doesn't validate key_len > 0 */
        if (rd_remaining(&r) < (int)key_len) break;
        rd_skip(&r, key_len);

        if (rd_remaining(&r) < 2) break;
        uint16_t val_len = rd_u16(&r);
        if (rd_remaining(&r) < (int)val_len) break;
        rd_skip(&r, val_len);
    }
    return PAX_OK;
}

int metadata_get_string(const pax_entry *e, const char *key,
                        char *value, size_t max_len) {
    if (!e->metadata || e->metadata_len == 0) return PAX_ERR_NOT_FOUND;
    pax_reader r;
    rd_init(&r, e->metadata, e->metadata_len);
    while (rd_remaining(&r) > 0) {
        uint8_t klen = rd_u8(&r);
        if (klen == 0 || rd_remaining(&r) < klen) break;
        const char *k = (const char*)rd_ptr(&r);
        rd_skip(&r, klen);
        if (rd_remaining(&r) < 2) break;
        uint16_t vlen = rd_u16(&r);
        if (rd_remaining(&r) < (int)vlen) break;
        const char *v = (const char*)rd_ptr(&r);
        rd_skip(&r, vlen);
        if (klen == (uint8_t)strlen(key) && memcmp(k, key, klen) == 0) {
            size_t cpy = vlen < max_len - 1 ? vlen : max_len - 1;
            memcpy(value, v, cpy);
            value[cpy] = '\0';
            return PAX_OK;
        }
    }
    return PAX_ERR_NOT_FOUND;
}
