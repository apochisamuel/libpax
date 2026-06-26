#include "types.h"
#include <string.h>
int glob_match(const char *pat, const char *str) {
    while (*pat && *str) {
        if (*pat == '*') {
            pat++;
            if (!*pat) return 1;
            while (*str) { if (glob_match(pat, str)) return 1; str++; }
            return 0;
        }
        if (*pat == '?') { pat++; str++; continue; }
        if (*pat != *str) return 0;
        pat++; str++;
    }
    while (*pat == '*') pat++;
    return *pat == '\0' && *str == '\0';
}
int search_entries(const pax_archive *ar, const char *pattern,
                   int *results, int max_results) {
    int found = 0;
    for (int i = 0; i < ar->entry_count && found < max_results; i++) {
        if (ar->entries[i].name && glob_match(pattern, ar->entries[i].name))
            results[found++] = i;
    }
    return found;
}
