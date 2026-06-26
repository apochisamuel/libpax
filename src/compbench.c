#include "types.h"
#include <stdlib.h>
#include <string.h>

/* Compression benchmarking: test all compression methods on a data
 * block and report compression ratio, verify round-trip correctness. */

typedef struct {
    uint8_t method;
    size_t original_size;
    size_t compressed_size;
    double ratio;
    int roundtrip_ok;
} comp_bench_result;

typedef struct {
    comp_bench_result results[4];  /* NONE, RLE, LZ, DELTA */
    int best_method;
    double best_ratio;
} comp_bench_report;

int benchmark_compression(const uint8_t *data, size_t len,
                          comp_bench_report *report) {
    if (!data || len == 0 || !report) return PAX_ERR_INVALID;
    memset(report, 0, sizeof(*report));
    report->best_ratio = 2.0;  /* worse than no compression */

    for (int method = 0; method < 4; method++) {
        comp_bench_result *r = &report->results[method];
        r->method = (uint8_t)method;
        r->original_size = len;

        if (method == PAX_COMP_NONE) {
            r->compressed_size = len;
            r->ratio = 1.0;
            r->roundtrip_ok = 1;
            if (r->ratio < report->best_ratio) {
                report->best_ratio = r->ratio;
                report->best_method = method;
            }
            continue;
        }

        uint8_t *compressed = NULL;
        size_t comp_len = 0;
        int rc;

        switch (method) {
            case PAX_COMP_RLE:
                rc = pax_rle_compress(data, len, &compressed, &comp_len);
                break;
            case PAX_COMP_LZ:
                rc = pax_lz_compress(data, len, &compressed, &comp_len);
                break;
            case PAX_COMP_DELTA:
                rc = pax_delta_encode(data, len, &compressed, &comp_len);
                break;
            default:
                rc = PAX_ERR_COMPRESS;
        }

        if (rc != PAX_OK || !compressed) {
            r->compressed_size = len;
            r->ratio = 1.0;
            r->roundtrip_ok = 0;
            continue;
        }

        r->compressed_size = comp_len;
        r->ratio = (double)comp_len / (double)len;

        /* Verify round-trip */
        uint8_t *decompressed = (uint8_t*)malloc(len);
        if (decompressed) {
            size_t written = 0;
            switch (method) {
                case PAX_COMP_RLE:
                    rc = pax_rle_decompress(compressed, comp_len,
                                            decompressed, len, &written);
                    break;
                case PAX_COMP_LZ:
                    rc = pax_lz_decompress(compressed, comp_len,
                                           decompressed, len, &written);
                    break;
                case PAX_COMP_DELTA:
                    rc = pax_delta_decode(compressed, comp_len,
                                          decompressed, len, &written);
                    break;
                default:
                    rc = PAX_ERR_COMPRESS;
            }
            r->roundtrip_ok = (rc == PAX_OK && written == len &&
                               memcmp(data, decompressed, len) == 0);
            free(decompressed);
        }

        free(compressed);

        if (r->ratio < report->best_ratio) {
            report->best_ratio = r->ratio;
            report->best_method = method;
        }
    }

    return PAX_OK;
}

/* Auto-select best compression for each entry in a builder */
int auto_compress_entries(pax_builder *b) {
    for (int i = 0; i < b->count; i++) {
        if (!b->data_blocks[i] || b->entries[i].compressed_size == 0) continue;

        comp_bench_report report;
        int rc = benchmark_compression(b->data_blocks[i],
                                        b->entries[i].compressed_size,
                                        &report);
        if (rc != PAX_OK) continue;

        if (report.best_method != PAX_COMP_NONE &&
            report.results[report.best_method].roundtrip_ok) {
            uint8_t *compressed = NULL;
            size_t comp_len = 0;

            switch (report.best_method) {
                case PAX_COMP_RLE:
                    pax_rle_compress(b->data_blocks[i],
                                    b->entries[i].compressed_size,
                                    &compressed, &comp_len);
                    break;
                case PAX_COMP_LZ:
                    pax_lz_compress(b->data_blocks[i],
                                   b->entries[i].compressed_size,
                                   &compressed, &comp_len);
                    break;
                case PAX_COMP_DELTA:
                    pax_delta_encode(b->data_blocks[i],
                                    b->entries[i].compressed_size,
                                    &compressed, &comp_len);
                    break;
            }

            if (compressed && comp_len < b->entries[i].compressed_size) {
                free(b->data_blocks[i]);
                b->data_blocks[i] = compressed;
                b->entries[i].compressed_size = (uint32_t)comp_len;
                b->entries[i].compression = (uint8_t)report.best_method;
            } else {
                free(compressed);
            }
        }
    }
    return PAX_OK;
}

/* Estimate total archive size with a given compression method */
size_t estimate_archive_size(const pax_builder *b, uint8_t method) {
    size_t total = 8 + 16;  /* magic + header */
    for (int i = 0; i < b->count; i++) {
        total += 2 + strlen(b->entries[i].name) + 21;  /* entry descriptor */
        if (b->entries[i].entry_type == PAX_ENTRY_SYMLINK && b->entries[i].symlink_target)
            total += 2 + strlen(b->entries[i].symlink_target);
        total += 2;  /* metadata len */
        /* Estimate compressed data size */
        if (method == PAX_COMP_NONE || !b->data_blocks[i])
            total += b->entries[i].compressed_size;
        else
            total += (size_t)(b->entries[i].compressed_size * 0.8);  /* estimate 80% */
    }
    return total;
}
