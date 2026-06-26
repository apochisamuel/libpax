#include "types.h"
#include <stdlib.h>
#include <string.h>
void buf_init(pax_buffer *b) { b->data=NULL; b->len=0; b->cap=0; }
int buf_append(pax_buffer *b, const uint8_t *d, size_t n) {
    if (b->len+n < b->len) return PAX_ERR_OVERFLOW;
    size_t need = b->len + n;
    if (need > b->cap) {
        size_t nc = b->cap ? b->cap : 256;
        while (nc < need) { if(nc>SIZE_MAX/2) return PAX_ERR_OVERFLOW; nc*=2; }
        uint8_t *p = (uint8_t*)realloc(b->data, nc);
        if(!p) return PAX_ERR_MEMORY;
        b->data=p; b->cap=nc;
    }
    memcpy(b->data+b->len, d, n); b->len+=n; return PAX_OK;
}
int buf_append_u8(pax_buffer *b, uint8_t v) { return buf_append(b,&v,1); }
int buf_append_u16(pax_buffer *b, uint16_t v) {
    uint8_t t[2]={(uint8_t)(v>>8),(uint8_t)v}; return buf_append(b,t,2);
}
int buf_append_u32(pax_buffer *b, uint32_t v) {
    uint8_t t[4]={(uint8_t)(v>>24),(uint8_t)(v>>16),(uint8_t)(v>>8),(uint8_t)v};
    return buf_append(b,t,4);
}
uint8_t *buf_detach(pax_buffer *b, size_t *out) { uint8_t*p=b->data; *out=b->len; b->data=NULL; b->len=b->cap=0; return p; }
void buf_free(pax_buffer *b) { free(b->data); b->data=NULL; b->len=b->cap=0; }
