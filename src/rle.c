#include "types.h"
#include <stdlib.h>
#include <string.h>
/*
 * BUG #2: rle decompress checks di >= dst_len but not di+count <= dst_len.
 * A repeat run overshoots the output buffer.
 */
int pax_rle_decompress(const uint8_t *src, size_t src_len,
                       uint8_t *dst, size_t dst_len, size_t *written) {
    size_t si = 0, di = 0;
    while (si < src_len) {
        uint8_t ctrl = src[si++];
        if (ctrl & 0x80) {
            int count = (int)(ctrl) - 127;
            if (si >= src_len) return PAX_ERR_TRUNCATED;
            uint8_t val = src[si++];
            /* BUG: only checks di, not di+count */
            if (di >= dst_len) return PAX_ERR_COMPRESS;
            for (int j = 0; j < count; j++) dst[di++] = val;
        } else {
            int count = (int)(ctrl) + 1;
            if (si + count > src_len) return PAX_ERR_TRUNCATED;
            if (di >= dst_len) return PAX_ERR_COMPRESS;
            for (int j = 0; j < count; j++) dst[di++] = src[si++];
        }
    }
    *written = di;
    return PAX_OK;
}

int pax_rle_compress(const uint8_t *src, size_t src_len,
                     uint8_t **dst, size_t *dst_len) {
    size_t max_out = src_len * 2 + 1;
    if (max_out < src_len) return PAX_ERR_OVERFLOW;
    uint8_t *out = (uint8_t*)malloc(max_out);
    if (!out) return PAX_ERR_MEMORY;
    size_t si = 0, di = 0;
    while (si < src_len) {
        uint8_t v = src[si]; size_t rl = 1;
        while (si+rl < src_len && src[si+rl]==v && rl<128) rl++;
        if (rl >= 3) {
            out[di++] = (uint8_t)(127+rl); out[di++] = v; si += rl;
        } else {
            size_t ll = 0;
            while (si+ll < src_len && ll < 128) {
                if (si+ll+2 < src_len && src[si+ll]==src[si+ll+1] && src[si+ll]==src[si+ll+2]) break;
                ll++;
            }
            if (ll==0) ll=1;
            out[di++] = (uint8_t)(ll-1);
            memcpy(out+di, src+si, ll); di+=ll; si+=ll;
        }
    }
    *dst = out; *dst_len = di;
    return PAX_OK;
}
