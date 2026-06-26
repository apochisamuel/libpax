#include "types.h"
#include <math.h>
#include <string.h>

typedef struct {
    int total_entries;
    int file_count;
    int dir_count;
    int symlink_count;
    uint64_t total_size;
    uint64_t total_compressed;
    double avg_ratio;
    uint32_t largest_file;
    uint32_t smallest_file;
    int comp_none, comp_rle, comp_lz, comp_delta;
} pax_stats;

void compute_archive_stats(const pax_archive *ar, pax_stats *s) {
    memset(s, 0, sizeof(*s));
    s->smallest_file = 0xFFFFFFFF;
    s->total_entries = ar->entry_count;
    
    for (int i = 0; i < ar->entry_count; i++) {
        const pax_entry *e = &ar->entries[i];
        switch (e->entry_type) {
            case PAX_ENTRY_FILE: s->file_count++; break;
            case PAX_ENTRY_DIR: s->dir_count++; break;
            case PAX_ENTRY_SYMLINK: s->symlink_count++; break;
        }
        s->total_size += e->size;
        s->total_compressed += e->compressed_size;
        if (e->size > s->largest_file) s->largest_file = e->size;
        if (e->size < s->smallest_file && e->entry_type == PAX_ENTRY_FILE)
            s->smallest_file = e->size;
        switch (e->compression) {
            case PAX_COMP_NONE: s->comp_none++; break;
            case PAX_COMP_RLE: s->comp_rle++; break;
            case PAX_COMP_LZ: s->comp_lz++; break;
            case PAX_COMP_DELTA: s->comp_delta++; break;
        }
    }
    if (s->total_size > 0)
        s->avg_ratio = (double)s->total_compressed / (double)s->total_size;
}

/* Compute Shannon entropy of data block */
double block_entropy(const uint8_t *data, size_t len) {
    if (len == 0) return 0.0;
    uint32_t freq[256] = {0};
    for (size_t i = 0; i < len; i++) freq[data[i]]++;
    double h = 0.0;
    for (int i = 0; i < 256; i++) {
        if (freq[i] > 0) {
            double p = (double)freq[i] / (double)len;
            h -= p * log2(p);
        }
    }
    return h;
}

/* Estimate compressibility based on entropy */
double estimate_compressibility(const uint8_t *data, size_t len) {
    double e = block_entropy(data, len);
    return 1.0 - (e / 8.0);  /* 8 bits max entropy */
}

/* Compute histogram of file sizes */
void size_histogram(const pax_archive *ar, uint32_t *buckets, int num_buckets,
                    uint32_t bucket_size) {
    memset(buckets, 0, num_buckets * sizeof(uint32_t));
    for (int i = 0; i < ar->entry_count; i++) {
        int b = (int)(ar->entries[i].size / bucket_size);
        if (b >= num_buckets) b = num_buckets - 1;
        buckets[b]++;
    }
}

/* Compute deduplication ratio */
double dedup_ratio(const pax_archive *ar) {
    if (ar->entry_count < 2) return 1.0;
    int unique = 0;
    for (int i = 0; i < ar->entry_count; i++) {
        int dup = 0;
        for (int j = 0; j < i; j++) {
            if (ar->entries[i].size == ar->entries[j].size &&
                ar->entries[i].checksum == ar->entries[j].checksum) {
                dup = 1; break;
            }
        }
        if (!dup) unique++;
    }
    return (double)unique / ar->entry_count;
}

/* Compute compression effectiveness per entry */
double entry_compression_ratio(const pax_entry *e) {
    if (e->size == 0) return 1.0;
    return (double)e->compressed_size / (double)e->size;
}

/* Variance of file sizes */
double size_variance(const pax_archive *ar) {
    if (ar->entry_count == 0) return 0.0;
    double sum = 0;
    for (int i = 0; i < ar->entry_count; i++) sum += ar->entries[i].size;
    double mean = sum / ar->entry_count;
    double var = 0;
    for (int i = 0; i < ar->entry_count; i++) {
        double d = ar->entries[i].size - mean;
        var += d * d;
    }
    return var / ar->entry_count;
}
