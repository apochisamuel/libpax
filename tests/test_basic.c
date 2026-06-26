#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "pax.h"
#include "types.h"
static int tr=0, tp=0;
#define T(n) do{printf("  %s...",#n);tr++;n();tp++;printf("OK\n");}while(0)
void test_crc(void) { assert(pax_crc32((uint8_t*)"hello",5) != 0); }
void test_adler(void) { assert(pax_adler32((uint8_t*)"hello",5) != 0); }
void test_error(void) { assert(strcmp(pax_strerror(PAX_OK),"no error")==0); }
void test_buffer(void) {
    pax_buffer b; buf_init(&b);
    buf_append_u8(&b, 0xFF); buf_append_u32(&b, 0x12345678);
    assert(b.len == 5); buf_free(&b);
}
void test_rle_rt(void) {
    uint8_t d[]={1,1,1,1,2,3,4,4,4,4,4}; uint8_t *c; size_t cl;
    assert(pax_rle_compress(d,sizeof(d),&c,&cl)==PAX_OK);
    uint8_t o[32]; size_t w;
    assert(pax_rle_decompress(c,cl,o,sizeof(o),&w)==PAX_OK);
    assert(w==sizeof(d) && memcmp(d,o,sizeof(d))==0); free(c);
}
void test_glob(void) { assert(glob_match("*.txt","hello.txt")); assert(!glob_match("*.c","hello.txt")); }
void test_perms(void) {
    char buf[16]; pax_format_permissions(0755, buf, 16);
    assert(strcmp(buf,"rwxr-xr-x")==0);
}
int main(void) {
    printf("libpax tests:\n");
    T(test_crc); T(test_adler); T(test_error);
    T(test_buffer); T(test_rle_rt); T(test_glob); T(test_perms);
    printf("\n%d/%d passed\n", tp, tr);
    return tp==tr ? 0 : 1;
}
