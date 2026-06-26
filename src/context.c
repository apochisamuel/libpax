#include "types.h"
#include <stdlib.h>
#include <string.h>
pax_archive *ctx_alloc(void) {
    pax_archive *ar = (pax_archive*)calloc(1, sizeof(pax_archive));
    if (!ar) return NULL;
    ar->entry_capacity = 32;
    ar->entries = (pax_entry*)calloc(ar->entry_capacity, sizeof(pax_entry));
    if (!ar->entries) { free(ar); return NULL; }
    return ar;
}
/*
 * BUG #8: ctx_free frees all entries including their data. But if
 * extract_entry failed mid-extraction, it may have already freed
 * some entry metadata. The pointers aren't NULLed, causing double-free.
 */
void ctx_free(pax_archive *ar) {
    if (!ar) return;
    if (ar->entries) {
        for (int i = 0; i < ar->entry_count; i++) free_entry(&ar->entries[i]);
        free(ar->entries);
    }
    if (ar->index) ht_destroy(ar->index);
    free(ar->decrypt_key);
    free(ar);
}
