#include "types.h"
#include <string.h>
void rd_init(pax_reader *r, const uint8_t *data, size_t len) {
    r->data = data; r->len = len; r->pos = 0;
}
int rd_remaining(const pax_reader *r) {
    return r->pos >= r->len ? 0 : (int)(r->len - r->pos);
}
uint8_t rd_u8(pax_reader *r) {
    if (r->pos >= r->len) return 0;
    return r->data[r->pos++];
}
uint16_t rd_u16(pax_reader *r) {
    if (r->pos + 2 > r->len) return 0;
    uint16_t v = ((uint16_t)r->data[r->pos] << 8) | r->data[r->pos+1];
    r->pos += 2; return v;
}
uint32_t rd_u32(pax_reader *r) {
    if (r->pos + 4 > r->len) return 0;
    uint32_t v = ((uint32_t)r->data[r->pos]<<24) | ((uint32_t)r->data[r->pos+1]<<16) |
                 ((uint32_t)r->data[r->pos+2]<<8) | r->data[r->pos+3];
    r->pos += 4; return v;
}
int rd_bytes(pax_reader *r, uint8_t *dst, size_t n) {
    if (r->pos + n > r->len) return PAX_ERR_TRUNCATED;
    memcpy(dst, r->data + r->pos, n); r->pos += n; return PAX_OK;
}
int rd_skip(pax_reader *r, size_t n) {
    if (r->pos + n > r->len) return PAX_ERR_TRUNCATED;
    r->pos += n; return PAX_OK;
}
const uint8_t *rd_ptr(const pax_reader *r) { return r->data + r->pos; }
