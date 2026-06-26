#include "types.h"
#include <stdlib.h>
#include <string.h>
/*
 * BUG #9: pax_xor_decrypt allocates output based on 'out_len' param
 * but the decrypt loop iterates over 'len' (input length). If the
 * entry's compressed_size (used as len) is larger than the declared
 * uncompressed size (used as out_len), the XOR loop writes past the
 * output buffer.
 */
int pax_xor_decrypt(const uint8_t *data, size_t len,
                    const uint8_t *key, size_t key_len,
                    uint8_t *out, size_t out_len) {
    if (!key || key_len == 0) return PAX_ERR_CRYPT;
    /* BUG: iterates over 'len' but output buffer is 'out_len' */
    for (size_t i = 0; i < len; i++) {
        out[i] = data[i] ^ key[i % key_len];
    }
    return PAX_OK;
}
int pax_xor_encrypt(const uint8_t *data, size_t len,
                    const uint8_t *key, size_t key_len,
                    uint8_t *out, size_t out_len) {
    if (!key || key_len == 0) return PAX_ERR_CRYPT;
    size_t n = len < out_len ? len : out_len;
    for (size_t i = 0; i < n; i++)
        out[i] = data[i] ^ key[i % key_len];
    return PAX_OK;
}
