#include "types.h"
#include <stdlib.h>
#include <string.h>

/* Streaming extraction - process archive entries without loading
 * the entire uncompressed data into memory at once. Uses a sliding
 * window buffer for decompression. */

#define STREAM_BUF_SIZE 4096

typedef struct {
    pax_archive *archive;
    const pax_entry *entry;
    const uint8_t *comp_data;
    size_t comp_len;
    size_t comp_pos;
    uint8_t buffer[STREAM_BUF_SIZE];
    size_t buf_start;
    size_t buf_end;
    size_t total_read;
    int finished;
    int error;
} pax_stream;

pax_stream *stream_open(pax_archive *ar, int entry_index) {
    const pax_entry *e = pax_get_entry(ar, entry_index);
    if (!e) return NULL;
    if (e->entry_type != PAX_ENTRY_FILE) return NULL;
    
    size_t dstart = (size_t)ar->header.data_offset + e->data_offset;
    if (dstart + e->compressed_size > ar->raw_len) return NULL;
    
    pax_stream *s = (pax_stream*)calloc(1, sizeof(pax_stream));
    if (!s) return NULL;
    s->archive = ar;
    s->entry = e;
    s->comp_data = ar->raw_data + dstart;
    s->comp_len = e->compressed_size;
    return s;
}

/* Read next chunk of decompressed data */
int stream_read(pax_stream *s, uint8_t *out, size_t max_bytes, size_t *bytes_read) {
    if (s->finished || s->error) {
        *bytes_read = 0;
        return s->error ? s->error : PAX_OK;
    }
    
    if (s->entry->compression == PAX_COMP_NONE) {
        /* Uncompressed: direct copy */
        size_t avail = s->comp_len - s->comp_pos;
        size_t n = avail < max_bytes ? avail : max_bytes;
        memcpy(out, s->comp_data + s->comp_pos, n);
        s->comp_pos += n;
        s->total_read += n;
        *bytes_read = n;
        if (s->comp_pos >= s->comp_len) s->finished = 1;
        return PAX_OK;
    }
    
    /* For compressed formats, decompress in chunks */
    if (s->buf_start >= s->buf_end) {
        /* Refill buffer */
        size_t remaining = s->comp_len - s->comp_pos;
        if (remaining == 0) { s->finished = 1; *bytes_read = 0; return PAX_OK; }
        size_t chunk = remaining < STREAM_BUF_SIZE ? remaining : STREAM_BUF_SIZE;
        
        size_t written = 0;
        int rc;
        if (s->entry->compression == PAX_COMP_RLE) {
            rc = pax_rle_decompress(s->comp_data + s->comp_pos, chunk,
                                    s->buffer, STREAM_BUF_SIZE, &written);
        } else if (s->entry->compression == PAX_COMP_LZ) {
            rc = pax_lz_decompress(s->comp_data + s->comp_pos, chunk,
                                   s->buffer, STREAM_BUF_SIZE, &written);
        } else if (s->entry->compression == PAX_COMP_DELTA) {
            rc = pax_delta_decode(s->comp_data + s->comp_pos, chunk,
                                  s->buffer, STREAM_BUF_SIZE, &written);
        } else {
            rc = PAX_ERR_COMPRESS;
        }
        
        if (rc != PAX_OK && rc != PAX_ERR_COMPRESS) {
            s->error = rc;
            *bytes_read = 0;
            return rc;
        }
        
        s->comp_pos += chunk;
        s->buf_start = 0;
        s->buf_end = written;
    }
    
    /* Copy from buffer to output */
    size_t avail = s->buf_end - s->buf_start;
    size_t n = avail < max_bytes ? avail : max_bytes;
    memcpy(out, s->buffer + s->buf_start, n);
    s->buf_start += n;
    s->total_read += n;
    *bytes_read = n;
    
    if (s->comp_pos >= s->comp_len && s->buf_start >= s->buf_end)
        s->finished = 1;
    
    return PAX_OK;
}

int stream_is_finished(const pax_stream *s) { return s->finished; }
size_t stream_bytes_read(const pax_stream *s) { return s->total_read; }
void stream_close(pax_stream *s) { free(s); }

/* Read entire entry via streaming */
int stream_read_all(pax_archive *ar, int entry_index,
                    uint8_t **out, size_t *out_len) {
    const pax_entry *e = pax_get_entry(ar, entry_index);
    if (!e) return PAX_ERR_NOT_FOUND;
    
    pax_stream *s = stream_open(ar, entry_index);
    if (!s) return PAX_ERR_MEMORY;
    
    pax_buffer buf;
    buf_init(&buf);
    
    uint8_t chunk[STREAM_BUF_SIZE];
    size_t nr;
    while (!stream_is_finished(s)) {
        int rc = stream_read(s, chunk, STREAM_BUF_SIZE, &nr);
        if (rc != PAX_OK) { buf_free(&buf); stream_close(s); return rc; }
        if (nr > 0) buf_append(&buf, chunk, nr);
    }
    
    stream_close(s);
    *out = buf_detach(&buf, out_len);
    return PAX_OK;
}
