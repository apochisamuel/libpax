#include "types.h"
#include <string.h>
uint16_t pax_parse_permissions(const char *str) {
    if (!str || strlen(str) < 9) return 0644;
    uint16_t p = 0;
    if (str[0]=='r') p|=0400; if (str[1]=='w') p|=0200; if (str[2]=='x') p|=0100;
    if (str[3]=='r') p|=0040; if (str[4]=='w') p|=0020; if (str[5]=='x') p|=0010;
    if (str[6]=='r') p|=0004; if (str[7]=='w') p|=0002; if (str[8]=='x') p|=0001;
    return p;
}
int pax_format_permissions(uint16_t p, char *buf, size_t buf_size) {
    if (buf_size < 10) return PAX_ERR_OVERFLOW;
    buf[0] = p&0400 ? 'r' : '-'; buf[1] = p&0200 ? 'w' : '-'; buf[2] = p&0100 ? 'x' : '-';
    buf[3] = p&0040 ? 'r' : '-'; buf[4] = p&0020 ? 'w' : '-'; buf[5] = p&0010 ? 'x' : '-';
    buf[6] = p&0004 ? 'r' : '-'; buf[7] = p&0002 ? 'w' : '-'; buf[8] = p&0001 ? 'x' : '-';
    buf[9] = '\0';
    return PAX_OK;
}
int pax_check_permission(uint16_t perm, uint8_t access) {
    if (access == 0) return perm & 0004 ? 1 : 0; /* read */
    if (access == 1) return perm & 0002 ? 1 : 0; /* write */
    if (access == 2) return perm & 0001 ? 1 : 0; /* exec */
    return 0;
}
