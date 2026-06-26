#include "types.h"
#include <stdlib.h>
#include <string.h>
/*
 * Simple LZ77-like decompression.
 * Tokens: [0xxxxxxx literal_byte] or [1xxxxxxx yyyyyyyy] = back-ref
 *   match_offset = x (7 bits, 1-128 back from current pos)
 *   match_length = y + 3
 *
 * BUG #3: back-reference can specify an offset larger than the
 * current output position, reading from before the buffer start.
 * This is an OOB read that can also become a write if the copied
 * data extends past dst_len.
 */
int pax_lz_decompress(const uint8_t *src, size_t src_len,
                      uint8_t *dst, size_t dst_len, size_t *written) {
    size_t si = 0, di = 0;
    while (si < src_len && di < dst_len) {
        uint8_t token = src[si++];
        if (token & 0x80) {
            /* Back-reference */
            int offset = (int)(token & 0x7F) + 1;
            if (si >= src_len) return PAX_ERR_TRUNCATED;
            int length = (int)src[si++] + 3;
            /*
             * BUG #3: no check that offset <= di.
             * If offset > di, we read before the start of dst.
             */
            size_t copy_from = di - (size_t)offset;
            for (int j = 0; j < length && di < dst_len; j++) {
                dst[di++] = dst[copy_from + j];
            }
        } else {
            /* Literal byte */
            dst[di++] = token;
        }
    }
    *written = di;
    return PAX_OK;
}

int pax_lz_compress(const uint8_t *src, size_t src_len,
                    uint8_t **dst, size_t *dst_len) {
    /* Simplified: just store as literals (no actual compression) */
    size_t max_out = src_len + 1;
    uint8_t *out = (uint8_t*)malloc(max_out);
    if (!out) return PAX_ERR_MEMORY;
    size_t di = 0;
    for (size_t i = 0; i < src_len; i++) {
        /* Check for repeats behind us */
        int best_off = 0, best_len = 0;
        for (int off = 1; off <= 128 && off <= (int)i; off++) {
            int ml = 0;
            while (i + ml < src_len && src[i+ml] == src[i-off+ml] && ml < 130) ml++;
            if (ml >= 3 && ml > best_len) { best_off = off; best_len = ml; }
        }
        if (best_len >= 3) {
            out[di++] = 0x80 | (uint8_t)(best_off - 1);
            out[di++] = (uint8_t)(best_len - 3);
            i += best_len - 1;
        } else {
            out[di++] = src[i] & 0x7F;  /* clear high bit for literal */
        }
    }
    *dst = out; *dst_len = di;
    return PAX_OK;
}
