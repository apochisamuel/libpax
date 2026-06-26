#include "types.h"
const char *pax_error_string(int c) {
    switch(c) {
        case PAX_OK: return "no error";
        case PAX_ERR_FORMAT: return "invalid format";
        case PAX_ERR_MEMORY: return "out of memory";
        case PAX_ERR_TRUNCATED: return "truncated data";
        case PAX_ERR_CHECKSUM: return "checksum mismatch";
        case PAX_ERR_OVERFLOW: return "integer overflow";
        case PAX_ERR_INVALID: return "invalid data";
        case PAX_ERR_DEPTH: return "nesting depth exceeded";
        case PAX_ERR_COMPRESS: return "compression error";
        case PAX_ERR_CRYPT: return "encryption error";
        case PAX_ERR_IO: return "I/O error";
        case PAX_ERR_NOT_FOUND: return "entry not found";
        default: return "unknown error";
    }
}
const char *pax_strerror(int c) { return pax_error_string(c); }
