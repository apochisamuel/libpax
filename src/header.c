#include "types.h"
#include <string.h>
int parse_archive_header(pax_reader *r, pax_header *hdr) {
    if (rd_remaining(r) < 16) return PAX_ERR_TRUNCATED;
    hdr->version = rd_u16(r);
    hdr->flags = rd_u16(r);
    hdr->entry_count = rd_u32(r);
    hdr->data_offset = rd_u32(r);
    hdr->header_checksum = rd_u32(r);
    if (hdr->version < 1 || hdr->version > 3) return PAX_ERR_FORMAT;
    if (hdr->entry_count > PAX_MAX_ENTRIES) return PAX_ERR_FORMAT;
    return PAX_OK;
}
int verify_header_checksum(const pax_header *hdr, const uint8_t *raw) {
    /* Checksum covers version+flags+entry_count+data_offset (12 bytes) */
    uint32_t computed = pax_crc32(raw + 8, 12);
    return computed == hdr->header_checksum ? PAX_OK : PAX_ERR_CHECKSUM;
}
