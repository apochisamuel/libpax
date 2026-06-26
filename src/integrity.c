#include "types.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    int entry_index;
    int error_code;
    const char *description;
} pax_integrity_issue;

typedef struct {
    pax_integrity_issue *issues;
    int count;
    int capacity;
} pax_integrity_report;

static int report_add(pax_integrity_report *rpt, int idx, int code, const char *desc) {
    if (rpt->count >= rpt->capacity) {
        int nc = rpt->capacity * 2;
        pax_integrity_issue *ni = (pax_integrity_issue*)realloc(rpt->issues,
            nc * sizeof(pax_integrity_issue));
        if (!ni) return PAX_ERR_MEMORY;
        rpt->issues = ni; rpt->capacity = nc;
    }
    rpt->issues[rpt->count].entry_index = idx;
    rpt->issues[rpt->count].error_code = code;
    rpt->issues[rpt->count].description = desc;
    rpt->count++;
    return PAX_OK;
}

int check_archive_integrity(const pax_archive *ar, pax_integrity_report *rpt) {
    rpt->count = 0;
    rpt->capacity = 16;
    rpt->issues = (pax_integrity_issue*)calloc(rpt->capacity, sizeof(pax_integrity_issue));
    if (!rpt->issues) return PAX_ERR_MEMORY;

    for (int i = 0; i < ar->entry_count; i++) {
        const pax_entry *e = &ar->entries[i];
        
        /* Check name */
        if (!e->name || strlen(e->name) == 0)
            report_add(rpt, i, PAX_ERR_INVALID, "empty name");
        
        /* Check for path traversal */
        if (e->name && !is_path_safe(e->name))
            report_add(rpt, i, PAX_ERR_INVALID, "unsafe path");
        
        /* Check data bounds */
        if (e->entry_type == PAX_ENTRY_FILE) {
            size_t dstart = (size_t)ar->header.data_offset + e->data_offset;
            if (dstart + e->compressed_size > ar->raw_len)
                report_add(rpt, i, PAX_ERR_TRUNCATED, "data out of bounds");
            
            /* Verify checksum */
            if (dstart + e->compressed_size <= ar->raw_len) {
                uint32_t crc = pax_crc32(ar->raw_data + dstart, e->compressed_size);
                if (crc != e->checksum)
                    report_add(rpt, i, PAX_ERR_CHECKSUM, "checksum mismatch");
            }
        }
        
        /* Check symlink has target */
        if (e->entry_type == PAX_ENTRY_SYMLINK && !e->symlink_target)
            report_add(rpt, i, PAX_ERR_INVALID, "symlink without target");
        
        /* Check for duplicate names */
        for (int j = 0; j < i; j++) {
            if (e->name && ar->entries[j].name &&
                strcmp(e->name, ar->entries[j].name) == 0) {
                report_add(rpt, i, PAX_ERR_INVALID, "duplicate name");
                break;
            }
        }
        
        /* Check compression type */
        if (e->compression > PAX_COMP_DELTA)
            report_add(rpt, i, PAX_ERR_FORMAT, "unknown compression");
        
        /* Check reasonable size ratio */
        if (e->size > 0 && e->compressed_size > e->size * 2)
            report_add(rpt, i, PAX_ERR_FORMAT, "suspicious compression ratio");
    }
    
    return PAX_OK;
}

void free_integrity_report(pax_integrity_report *rpt) {
    free(rpt->issues);
    rpt->issues = NULL;
    rpt->count = 0;
}

/* Quick check - returns first error found */
int quick_integrity_check(const pax_archive *ar) {
    for (int i = 0; i < ar->entry_count; i++) {
        int rc = validate_entry(&ar->entries[i], ar->raw_data, ar->raw_len);
        if (rc != PAX_OK) return rc;
    }
    return PAX_OK;
}
