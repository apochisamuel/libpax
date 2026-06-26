#include "types.h"
#include <stdlib.h>
#include <string.h>
/*
 * BUG #4: Recursive extraction for nested archives (entry_type check
 * is wrong — it recurses when data starts with PAX_MAGIC, without
 * a depth limit in practice because depth is checked too late).
 *
 * BUG #8: On extraction failure, entry metadata is freed but the
 * pointer in ar->entries is not NULLed. ctx_free double-frees it.
 */
int extract_entry(pax_archive *ar, const pax_entry *e,
                  uint8_t **out, size_t *out_len, int depth) {
    if (depth > PAX_MAX_NEST_DEPTH) return PAX_ERR_DEPTH;
    if (e->entry_type == PAX_ENTRY_DIR) return PAX_ERR_INVALID;

    /* Validate offset + compressed_size within archive bounds */
    size_t dstart = (size_t)ar->header.data_offset + e->data_offset;
    if (dstart + e->compressed_size > ar->raw_len)
        return PAX_ERR_TRUNCATED;

    const uint8_t *comp_data = ar->raw_data + dstart;
    size_t comp_len = e->compressed_size;

    /* Decrypt if archive is encrypted */
    uint8_t *decrypted = NULL;
    if (ar->header.flags & PAX_FLAG_ENCRYPTED) {
        if (!ar->decrypt_key || ar->key_len == 0)
            return PAX_ERR_CRYPT;
        /* BUG #9 setup: allocate based on uncompressed size,
         * decrypt loop uses compressed size */
        decrypted = (uint8_t*)malloc(e->size > 0 ? e->size : 1);
        if (!decrypted) return PAX_ERR_MEMORY;
        int rc = pax_xor_decrypt(comp_data, comp_len,
                                  ar->decrypt_key, ar->key_len,
                                  decrypted, e->size);
        if (rc != PAX_OK) { free(decrypted); return rc; }
        comp_data = decrypted;
        comp_len = e->size;  /* after decrypt, use uncompressed length */
    }

    uint8_t *result = (uint8_t*)malloc(e->size > 0 ? e->size : 1);
    if (!result) { free(decrypted); return PAX_ERR_MEMORY; }

    size_t written = 0;
    int rc;
    switch (e->compression) {
        case PAX_COMP_NONE:
            { size_t n = comp_len < e->size ? comp_len : e->size;
              memcpy(result, comp_data, n); written = n; rc = PAX_OK; }
            break;
        case PAX_COMP_RLE:
            rc = pax_rle_decompress(comp_data, comp_len, result, e->size, &written);
            break;
        case PAX_COMP_LZ:
            rc = pax_lz_decompress(comp_data, comp_len, result, e->size, &written);
            break;
        case PAX_COMP_DELTA:
            rc = pax_delta_decode(comp_data, comp_len, result, e->size, &written);
            break;
        default:
            rc = PAX_ERR_COMPRESS;
    }

    free(decrypted);

    if (rc != PAX_OK) {
        free(result);
        /*
         * BUG #8: free metadata on failed extraction, but don't NULL
         * the pointer. ctx_free will free it again.
         */
        if (e->metadata) {
            free(e->metadata);
            /* Missing: ((pax_entry*)e)->metadata = NULL; */
        }
        return rc;
    }

    /* Check if extracted data is itself a PAX archive (nested) */
    if (written >= 8 && memcmp(result, PAX_MAGIC, 8) == 0) {
        /* BUG #4: recurse into nested archive */
        pax_archive *nested = pax_open(result, written);
        if (nested && pax_entry_count(nested) > 0) {
            uint8_t *inner = NULL;
            size_t inner_len = 0;
            extract_entry(nested, pax_get_entry(nested, 0),
                          &inner, &inner_len, depth + 1);
            pax_close(nested);
            if (inner) { free(result); result = inner; written = inner_len; }
        } else {
            if (nested) pax_close(nested);
        }
    }

    *out = result;
    *out_len = written;
    return PAX_OK;
}
