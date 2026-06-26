#ifndef PAX_TYPES_H
#define PAX_TYPES_H
#include "pax.h"
#include <stdint.h>
#include <stddef.h>

static const uint8_t PAX_MAGIC[8] = {0x89, 0x50, 0x41, 0x58, 0x0D, 0x0A, 0x1A, 0x0A};

#define PAX_MAX_ENTRIES     4096
#define PAX_MAX_NAME_LEN    1024
#define PAX_MAX_PATH_LEN    4096
#define PAX_MAX_NEST_DEPTH  16
#define PAX_MAX_KEY_SIZE    256
#define PAX_HASH_BUCKETS    251

#define PAX_ENTRY_FILE      0
#define PAX_ENTRY_DIR       1
#define PAX_ENTRY_SYMLINK   2

/* Binary reader */
typedef struct {
    const uint8_t *data;
    size_t len;
    size_t pos;
} pax_reader;

/* Archive header (on disk) */
typedef struct {
    uint16_t version;
    uint16_t flags;
    uint32_t entry_count;
    uint32_t data_offset;
    uint32_t header_checksum;
} pax_header;

/* Entry descriptor */
struct pax_entry {
    char    *name;          /* heap-allocated filename */
    uint32_t size;          /* uncompressed size */
    uint32_t compressed_size;
    uint32_t data_offset;   /* offset into archive data section */
    uint8_t  compression;
    uint8_t  entry_type;    /* file, dir, symlink */
    uint32_t checksum;
    uint32_t timestamp;
    uint16_t permissions;
    char    *symlink_target; /* for symlinks */
    /* Metadata */
    uint8_t *metadata;
    uint16_t metadata_len;
};

/* Hash table for name lookups */
typedef struct pax_hash_node {
    const char *key;
    int entry_index;
    struct pax_hash_node *next;
} pax_hash_node;

typedef struct {
    pax_hash_node *buckets[PAX_HASH_BUCKETS];
    int count;
} pax_hash_table;

/* Archive context */
struct pax_archive {
    const uint8_t *raw_data;
    size_t raw_len;
    pax_header header;
    pax_entry *entries;
    int entry_count;
    int entry_capacity;
    pax_hash_table *index;
    uint8_t *decrypt_key;
    size_t key_len;
    int error;
};

/* Builder context */
struct pax_builder {
    pax_entry *entries;
    uint8_t **data_blocks;
    int count;
    int capacity;
};

/* Iterator */
struct pax_iterator {
    const pax_archive *archive;
    int position;
};

/* Internal functions */

/* reader.c */
void rd_init(pax_reader *r, const uint8_t *data, size_t len);
int rd_remaining(const pax_reader *r);
uint8_t rd_u8(pax_reader *r);
uint16_t rd_u16(pax_reader *r);
uint32_t rd_u32(pax_reader *r);
int rd_bytes(pax_reader *r, uint8_t *dst, size_t n);
int rd_skip(pax_reader *r, size_t n);
const uint8_t *rd_ptr(const pax_reader *r);

/* checksum.c */
uint32_t pax_crc32(const uint8_t *data, size_t len);
uint32_t pax_crc32_update(uint32_t crc, const uint8_t *data, size_t len);
uint32_t pax_adler32(const uint8_t *data, size_t len);

/* header.c */
int parse_archive_header(pax_reader *r, pax_header *hdr);
int verify_header_checksum(const pax_header *hdr, const uint8_t *raw);

/* entry.c */
int parse_entry_table(pax_archive *ar, pax_reader *r);
void free_entry(pax_entry *e);
void free_entries(pax_entry *entries, int count);

/* filename.c */
int normalize_path(const char *input, char *output, size_t out_size);
int path_join(const char *base, const char *rel, char *out, size_t out_size);
int path_depth(const char *path);
int is_path_safe(const char *path);
const char *path_basename(const char *path);
const char *path_extension(const char *path);

/* rle.c */
int pax_rle_decompress(const uint8_t *src, size_t src_len,
                       uint8_t *dst, size_t dst_len, size_t *written);
int pax_rle_compress(const uint8_t *src, size_t src_len,
                     uint8_t **dst, size_t *dst_len);

/* lz.c */
int pax_lz_decompress(const uint8_t *src, size_t src_len,
                      uint8_t *dst, size_t dst_len, size_t *written);
int pax_lz_compress(const uint8_t *src, size_t src_len,
                    uint8_t **dst, size_t *dst_len);

/* delta.c */
int pax_delta_decode(const uint8_t *src, size_t src_len,
                     uint8_t *dst, size_t dst_len, size_t *written);
int pax_delta_encode(const uint8_t *src, size_t src_len,
                     uint8_t **dst, size_t *dst_len);

/* crypt.c */
int pax_xor_decrypt(const uint8_t *data, size_t len,
                    const uint8_t *key, size_t key_len,
                    uint8_t *out, size_t out_len);
int pax_xor_encrypt(const uint8_t *data, size_t len,
                    const uint8_t *key, size_t key_len,
                    uint8_t *out, size_t out_len);

/* extract.c */
int extract_entry(pax_archive *ar, const pax_entry *e,
                  uint8_t **out, size_t *out_len, int depth);

/* metadata.c */
int parse_entry_metadata(const uint8_t *data, size_t len,
                         pax_entry *entry);
int metadata_get_string(const pax_entry *e, const char *key,
                        char *value, size_t max_len);

/* validate.c */
int validate_archive(const pax_archive *ar);
int validate_entry(const pax_entry *e, const uint8_t *archive_data, size_t archive_len);
int verify_entry_checksum(const pax_entry *e, const uint8_t *data);

/* hashtable.c */
pax_hash_table *ht_create(void);
void ht_destroy(pax_hash_table *ht);
int ht_insert(pax_hash_table *ht, const char *key, int value);
int ht_lookup(const pax_hash_table *ht, const char *key);
uint32_t ht_hash(const char *key);

/* buffer.c */
typedef struct {
    uint8_t *data;
    size_t len, cap;
} pax_buffer;
void buf_init(pax_buffer *b);
int buf_append(pax_buffer *b, const uint8_t *data, size_t len);
int buf_append_u8(pax_buffer *b, uint8_t v);
int buf_append_u16(pax_buffer *b, uint16_t v);
int buf_append_u32(pax_buffer *b, uint32_t v);
uint8_t *buf_detach(pax_buffer *b, size_t *out_len);
void buf_free(pax_buffer *b);

/* sort.c */
void sort_entries_by_name(pax_entry *entries, int count);
void sort_entries_by_size(pax_entry *entries, int count);
void sort_entries_by_offset(pax_entry *entries, int count);
int binary_search_entry(const pax_entry *entries, int count, const char *name);

/* error.c */
const char *pax_error_string(int code);

/* context.c */
pax_archive *ctx_alloc(void);
void ctx_free(pax_archive *ar);

/* merge.c */
int merge_archives(const pax_archive *a, const pax_archive *b,
                   uint8_t **out, size_t *out_len);
int count_unique_entries(const pax_archive *a, const pax_archive *b);

/* search.c */
int glob_match(const char *pattern, const char *str);
int search_entries(const pax_archive *ar, const char *pattern,
                   int *results, int max_results);

/* timestamp.c */
typedef struct {
    int year, month, day, hour, minute, second;
} pax_time;
uint32_t pax_time_to_unix(const pax_time *t);
void pax_unix_to_time(uint32_t ts, pax_time *out);
int pax_time_compare(const pax_time *a, const pax_time *b);
int pax_time_format(uint32_t ts, char *buf, size_t buf_size);

/* permissions.c */
uint16_t pax_parse_permissions(const char *str);
int pax_format_permissions(uint16_t perm, char *buf, size_t buf_size);
int pax_check_permission(uint16_t perm, uint8_t access_type);

#endif
