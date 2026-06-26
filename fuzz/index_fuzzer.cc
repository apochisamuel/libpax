#include <cstdint>
#include <cstddef>
#include <cstdlib>
extern "C" {
#include "pax.h"
}
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size < 48) return 0;
    /* Try merging two copies */
    size_t half = size / 2;
    pax_archive *a = pax_open(data, half);
    pax_archive *b = pax_open(data + half, size - half);
    if (a && b) {
        uint8_t *merged = NULL; size_t mlen = 0;
        if (pax_merge(a, b, &merged, &mlen) == 0 && merged) free(merged);
    }
    if (a) pax_close(a);
    if (b) pax_close(b);
    return 0;
}
