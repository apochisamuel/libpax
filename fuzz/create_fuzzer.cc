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
    /* Try to re-serialize and reparse */
    pax_builder *b = pax_builder_create();
    if (b) {
        for (int i = 0; i < n && i < 4; i++) {
            const pax_entry *e = pax_get_entry(ar, i);
            if (e && pax_entry_name(e)) {
                uint8_t *out = NULL; size_t out_len = 0;
                if (pax_extract(ar, i, &out, &out_len) == 0 && out) {
                    pax_builder_add(b, pax_entry_name(e), out, out_len, 0);
                    free(out);
                }
            }
        }
        uint8_t *ser = NULL; size_t ser_len = 0;
        if (pax_builder_finish(b, &ser, &ser_len) == 0 && ser) {
            pax_archive *ar2 = pax_open(ser, ser_len);
            if (ar2) pax_close(ar2);
            free(ser);
        }
        pax_builder_free(b);
    }
    pax_close(ar);
    return 0;
}
