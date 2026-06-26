#include <cstdint>
#include <cstddef>
#include <cstdlib>
extern "C" {
#include "pax.h"
}
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size < 24) return 0;
    pax_archive *ar = pax_open(data, size);
    if (!ar) return 0;
    int n = pax_entry_count(ar);
    for (int i = 0; i < n && i < 8; i++) {
        uint8_t *out = NULL; size_t out_len = 0;
        if (pax_extract(ar, i, &out, &out_len) == 0 && out) free(out);
    }
    (void)pax_validate(ar);
    pax_close(ar);
    return 0;
}
