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
    pax_build_index(ar);
    int n = pax_entry_count(ar);
    for (int i = 0; i < n && i < 4; i++) {
        const pax_entry *e = pax_get_entry(ar, i);
        if (e && pax_entry_name(e))
            (void)pax_find_entry(ar, pax_entry_name(e));
    }
    int results[16]; pax_search(ar, "*", results, 16);
    pax_close(ar);
    return 0;
}
