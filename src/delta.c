#include "types.h"
#include <stdlib.h>
/* Delta encoding: each byte stores the difference from the previous.
 * First byte stored as-is. Useful for sorted/sequential data. */
int pax_delta_decode(const uint8_t *src, size_t src_len,
                     uint8_t *dst, size_t dst_len, size_t *written) {
    if (src_len == 0) { *written = 0; return PAX_OK; }
    size_t n = src_len < dst_len ? src_len : dst_len;
    dst[0] = src[0];
    for (size_t i = 1; i < n; i++)
        dst[i] = (uint8_t)(dst[i-1] + src[i]);
    *written = n;
    return PAX_OK;
}
int pax_delta_encode(const uint8_t *src, size_t src_len,
                     uint8_t **dst, size_t *dst_len) {
    uint8_t *out = (uint8_t*)malloc(src_len);
    if (!out) return PAX_ERR_MEMORY;
    if (src_len > 0) {
        out[0] = src[0];
        for (size_t i = 1; i < src_len; i++)
            out[i] = (uint8_t)(src[i] - src[i-1]);
    }
    *dst = out; *dst_len = src_len;
    return PAX_OK;
}
