#include "types.h"
static uint32_t crc_tab[256];
static int crc_ready = 0;
static void crc_init(void) {
    for (int i = 0; i < 256; i++) {
        uint32_t c = (uint32_t)i;
        for (int k = 0; k < 8; k++) c = c&1 ? 0xEDB88320U^(c>>1) : c>>1;
        crc_tab[i] = c;
    }
    crc_ready = 1;
}
uint32_t pax_crc32_update(uint32_t crc, const uint8_t *d, size_t len) {
    if (!crc_ready) crc_init();
    for (size_t i = 0; i < len; i++) crc = crc_tab[(crc^d[i])&0xFF]^(crc>>8);
    return crc;
}
uint32_t pax_crc32(const uint8_t *d, size_t len) {
    return pax_crc32_update(0xFFFFFFFFU, d, len) ^ 0xFFFFFFFFU;
}
uint32_t pax_adler32(const uint8_t *d, size_t len) {
    uint32_t a = 1, b = 0;
    for (size_t i = 0; i < len; i++) { a = (a + d[i]) % 65521; b = (b + a) % 65521; }
    return (b << 16) | a;
}
