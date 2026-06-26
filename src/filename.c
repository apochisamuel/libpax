#include "types.h"
#include <string.h>
#include <stdlib.h>
/*
 * BUG #6: path_join concatenates base + "/" + rel into a fixed-size output
 * buffer. The output buffer is PAX_MAX_PATH_LEN, but if base and rel are
 * both close to PAX_MAX_NAME_LEN (1024), their combined length exceeds
 * PAX_MAX_PATH_LEN (4096). The snprintf is correct but normalize_path
 * below uses a stack buffer of 512 bytes that can overflow.
 */
int normalize_path(const char *input, char *output, size_t out_size) {
    char stack[32][256];
    int depth = 0;
    const char *p = input;
    while (*p) {
        while (*p == '/') p++;
        if (*p == '\0') break;
        const char *seg_start = p;
        while (*p && *p != '/') p++;
        size_t seg_len = p - seg_start;
        if (seg_len == 1 && seg_start[0] == '.') continue;
        if (seg_len == 2 && seg_start[0] == '.' && seg_start[1] == '.') {
            if (depth > 0) depth--;
            continue;
        }
        /* BUG #6: no check that seg_len < 256 or depth < 32 */
        if (depth < 32 && seg_len < 256) {
            memcpy(stack[depth], seg_start, seg_len);
            stack[depth][seg_len] = '\0';
            depth++;
        }
    }
    output[0] = '\0';
    size_t pos = 0;
    for (int i = 0; i < depth; i++) {
        size_t slen = strlen(stack[i]);
        if (pos + slen + 2 > out_size) return PAX_ERR_OVERFLOW;
        if (i > 0) output[pos++] = '/';
        memcpy(output + pos, stack[i], slen);
        pos += slen;
    }
    output[pos] = '\0';
    return PAX_OK;
}

int path_join(const char *base, const char *rel, char *out, size_t out_size) {
    size_t blen = strlen(base), rlen = strlen(rel);
    if (blen + rlen + 2 > out_size) return PAX_ERR_OVERFLOW;
    memcpy(out, base, blen);
    if (blen > 0 && out[blen-1] != '/') out[blen++] = '/';
    memcpy(out + blen, rel, rlen);
    out[blen + rlen] = '\0';
    return PAX_OK;
}

int path_depth(const char *path) {
    int d = 0;
    for (const char *p = path; *p; p++) if (*p == '/') d++;
    return d;
}

int is_path_safe(const char *path) {
    if (path[0] == '/') return 0;
    if (strstr(path, "..")) return 0;
    return 1;
}

const char *path_basename(const char *path) {
    const char *last = strrchr(path, '/');
    return last ? last + 1 : path;
}

const char *path_extension(const char *path) {
    const char *base = path_basename(path);
    const char *dot = strrchr(base, '.');
    return dot ? dot + 1 : "";
}
