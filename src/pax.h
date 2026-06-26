#ifndef PAX_H
#define PAX_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

#define PAX_OK              0
#define PAX_ERR_FORMAT     -1
#define PAX_ERR_MEMORY     -2
#define PAX_ERR_TRUNCATED  -3
#define PAX_ERR_CHECKSUM   -4
#define PAX_ERR_OVERFLOW   -5
#define PAX_ERR_INVALID    -6
#define PAX_ERR_DEPTH      -7
#define PAX_ERR_COMPRESS   -8
#define PAX_ERR_CRYPT      -9
#define PAX_ERR_IO        -10
#define PAX_ERR_NOT_FOUND -11

#define PAX_COMP_NONE      0
#define PAX_COMP_RLE       1
#define PAX_COMP_LZ        2
#define PAX_COMP_DELTA     3

#define PAX_FLAG_ENCRYPTED   0x01
#define PAX_FLAG_SIGNED      0x02
#define PAX_FLAG_SOLID       0x04

typedef struct pax_archive pax_archive;
typedef struct pax_entry pax_entry;
typedef struct pax_iterator pax_iterator;

pax_archive *pax_open(const uint8_t *data, size_t len);
void pax_close(pax_archive *ar);
int pax_entry_count(const pax_archive *ar);
const pax_entry *pax_get_entry(const pax_archive *ar, int index);
const pax_entry *pax_find_entry(const pax_archive *ar, const char *name);
int pax_extract(pax_archive *ar, int index, uint8_t **out, size_t *out_len);
int pax_extract_by_name(pax_archive *ar, const char *name, uint8_t **out, size_t *out_len);
int pax_validate(const pax_archive *ar);
const char *pax_entry_name(const pax_entry *e);
uint32_t pax_entry_size(const pax_entry *e);
uint32_t pax_entry_compressed_size(const pax_entry *e);
uint8_t pax_entry_compression(const pax_entry *e);
uint32_t pax_entry_timestamp(const pax_entry *e);
uint16_t pax_entry_permissions(const pax_entry *e);
uint32_t pax_entry_checksum(const pax_entry *e);
int pax_entry_is_directory(const pax_entry *e);
int pax_entry_is_symlink(const pax_entry *e);
const char *pax_entry_symlink_target(const pax_entry *e);

/* Archive creation */
typedef struct pax_builder pax_builder;
pax_builder *pax_builder_create(void);
int pax_builder_add(pax_builder *b, const char *name, const uint8_t *data,
                    size_t len, uint8_t compression);
int pax_builder_add_dir(pax_builder *b, const char *name);
int pax_builder_add_symlink(pax_builder *b, const char *name, const char *target);
int pax_builder_finish(pax_builder *b, uint8_t **out, size_t *out_len);
void pax_builder_free(pax_builder *b);

/* Merge/index */
int pax_merge(const pax_archive *a, const pax_archive *b, uint8_t **out, size_t *out_len);
int pax_build_index(pax_archive *ar);
int pax_search(const pax_archive *ar, const char *pattern, int *results, int max_results);

/* Iterator */
pax_iterator *pax_iter_create(const pax_archive *ar);
const pax_entry *pax_iter_next(pax_iterator *it);
void pax_iter_free(pax_iterator *it);

/* Encryption */
int pax_set_key(pax_archive *ar, const uint8_t *key, size_t key_len);

/* Utilities */
const char *pax_strerror(int code);
void pax_free_buffer(uint8_t *buf);

#ifdef __cplusplus
}
#endif
#endif
