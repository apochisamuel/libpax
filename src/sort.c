#include "types.h"
#include <string.h>
#include <stdlib.h>
static int cmp_name(const void *a, const void *b) {
    const pax_entry *ea = (const pax_entry*)a, *eb = (const pax_entry*)b;
    if (!ea->name) return eb->name ? -1 : 0;
    if (!eb->name) return 1;
    return strcmp(ea->name, eb->name);
}
static int cmp_size(const void *a, const void *b) {
    const pax_entry *ea = (const pax_entry*)a, *eb = (const pax_entry*)b;
    return ea->size < eb->size ? -1 : ea->size > eb->size ? 1 : 0;
}
static int cmp_offset(const void *a, const void *b) {
    const pax_entry *ea = (const pax_entry*)a, *eb = (const pax_entry*)b;
    return ea->data_offset < eb->data_offset ? -1 : ea->data_offset > eb->data_offset ? 1 : 0;
}
void sort_entries_by_name(pax_entry *entries, int count) {
    qsort(entries, count, sizeof(pax_entry), cmp_name);
}
void sort_entries_by_size(pax_entry *entries, int count) {
    qsort(entries, count, sizeof(pax_entry), cmp_size);
}
void sort_entries_by_offset(pax_entry *entries, int count) {
    qsort(entries, count, sizeof(pax_entry), cmp_offset);
}
int binary_search_entry(const pax_entry *entries, int count, const char *name) {
    int lo = 0, hi = count - 1;
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;
        int c = strcmp(entries[mid].name, name);
        if (c == 0) return mid;
        if (c < 0) lo = mid + 1; else hi = mid - 1;
    }
    return -1;
}
