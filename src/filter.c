#include "types.h"
#include <string.h>
#include <stdlib.h>

/* Boyer-Moore-Horspool string search within extracted entry data */
typedef struct {
    int entry_index;
    size_t offset;
    size_t match_len;
} pax_match;

static void build_bad_char_table(const uint8_t *pattern, size_t pat_len,
                                  int table[256]) {
    for (int i = 0; i < 256; i++) table[i] = (int)pat_len;
    for (size_t i = 0; i < pat_len - 1; i++)
        table[pattern[i]] = (int)(pat_len - 1 - i);
}

int search_in_data(const uint8_t *data, size_t data_len,
                   const uint8_t *pattern, size_t pat_len,
                   pax_match *matches, int max_matches) {
    if (pat_len == 0 || pat_len > data_len) return 0;
    
    int bad_char[256];
    build_bad_char_table(pattern, pat_len, bad_char);
    
    int found = 0;
    size_t i = 0;
    while (i <= data_len - pat_len && found < max_matches) {
        size_t j = pat_len - 1;
        while (data[i + j] == pattern[j]) {
            if (j == 0) {
                matches[found].offset = i;
                matches[found].match_len = pat_len;
                found++;
                break;
            }
            j--;
        }
        i += (size_t)bad_char[data[i + pat_len - 1]];
    }
    return found;
}

/* Search across all entries in an archive */
int search_archive_contents(pax_archive *ar, const uint8_t *pattern,
                            size_t pat_len, pax_match *matches,
                            int max_matches) {
    int total = 0;
    for (int i = 0; i < ar->entry_count && total < max_matches; i++) {
        if (ar->entries[i].entry_type != PAX_ENTRY_FILE) continue;
        
        uint8_t *data = NULL;
        size_t data_len = 0;
        int rc = pax_extract(ar, i, &data, &data_len);
        if (rc != PAX_OK || !data) continue;
        
        pax_match local[64];
        int found = search_in_data(data, data_len, pattern, pat_len,
                                   local, max_matches - total < 64 ?
                                   max_matches - total : 64);
        for (int j = 0; j < found && total < max_matches; j++) {
            matches[total] = local[j];
            matches[total].entry_index = i;
            total++;
        }
        free(data);
    }
    return total;
}

/* Count occurrences of a byte pattern */
int count_pattern(const uint8_t *data, size_t len,
                  const uint8_t *pat, size_t pat_len) {
    if (pat_len == 0 || pat_len > len) return 0;
    int count = 0;
    for (size_t i = 0; i <= len - pat_len; i++) {
        if (memcmp(data + i, pat, pat_len) == 0) count++;
    }
    return count;
}

/* Simple byte frequency analysis */
void byte_frequency(const uint8_t *data, size_t len, double freq[256]) {
    uint32_t counts[256] = {0};
    for (size_t i = 0; i < len; i++) counts[data[i]]++;
    for (int i = 0; i < 256; i++)
        freq[i] = len > 0 ? (double)counts[i] / (double)len : 0.0;
}
