#include "types.h"
#include <string.h>
#include <stdlib.h>

typedef struct {
    const char *name;
    int status;  /* 0=same, 1=added, 2=removed, 3=modified */
    uint32_t size_a, size_b;
} pax_diff_entry;

typedef struct {
    pax_diff_entry *entries;
    int count;
    int added, removed, modified, unchanged;
} pax_diff_result;

int compute_archive_diff(const pax_archive *a, const pax_archive *b,
                         pax_diff_result *result) {
    int max = a->entry_count + b->entry_count;
    result->entries = (pax_diff_entry*)calloc(max, sizeof(pax_diff_entry));
    if (!result->entries) return PAX_ERR_MEMORY;
    result->count = 0;
    result->added = result->removed = result->modified = result->unchanged = 0;

    /* Check entries in A */
    for (int i = 0; i < a->entry_count; i++) {
        pax_diff_entry *de = &result->entries[result->count++];
        de->name = a->entries[i].name;
        de->size_a = a->entries[i].size;
        
        /* Find in B */
        int found = 0;
        for (int j = 0; j < b->entry_count; j++) {
            if (b->entries[j].name && a->entries[i].name &&
                strcmp(a->entries[i].name, b->entries[j].name) == 0) {
                de->size_b = b->entries[j].size;
                if (a->entries[i].checksum == b->entries[j].checksum &&
                    a->entries[i].size == b->entries[j].size) {
                    de->status = 0; result->unchanged++;
                } else {
                    de->status = 3; result->modified++;
                }
                found = 1; break;
            }
        }
        if (!found) { de->status = 2; result->removed++; }
    }

    /* Check entries in B not in A */
    for (int j = 0; j < b->entry_count; j++) {
        int found = 0;
        for (int i = 0; i < a->entry_count; i++) {
            if (a->entries[i].name && b->entries[j].name &&
                strcmp(a->entries[i].name, b->entries[j].name) == 0) {
                found = 1; break;
            }
        }
        if (!found) {
            pax_diff_entry *de = &result->entries[result->count++];
            de->name = b->entries[j].name;
            de->size_b = b->entries[j].size;
            de->status = 1; result->added++;
        }
    }
    return PAX_OK;
}

void free_diff_result(pax_diff_result *r) {
    free(r->entries);
    r->entries = NULL;
    r->count = 0;
}

/* Check if two archives are identical by content */
int archives_equal(const pax_archive *a, const pax_archive *b) {
    if (a->entry_count != b->entry_count) return 0;
    for (int i = 0; i < a->entry_count; i++) {
        int found = 0;
        for (int j = 0; j < b->entry_count; j++) {
            if (a->entries[i].name && b->entries[j].name &&
                strcmp(a->entries[i].name, b->entries[j].name) == 0 &&
                a->entries[i].size == b->entries[j].size &&
                a->entries[i].checksum == b->entries[j].checksum) {
                found = 1; break;
            }
        }
        if (!found) return 0;
    }
    return 1;
}
