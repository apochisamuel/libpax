#include "types.h"
#include <stdio.h>
/*
 * BUG #12: pax_time_to_unix uses int arithmetic for year/month
 * calculations that can overflow for extreme timestamp values,
 * producing negative or wrapped values.
 */
uint32_t pax_time_to_unix(const pax_time *t) {
    /* Days from epoch (1970-01-01) - simplified, no leap second handling */
    int days_in_month[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    int y = t->year, m = t->month, d = t->day;

    /* BUG #12: no validation on year/month range */
    int total_days = 0;
    for (int yr = 1970; yr < y; yr++) {
        int leap = (yr%4==0 && (yr%100!=0 || yr%400==0)) ? 1 : 0;
        total_days += 365 + leap;
    }
    int leap = (y%4==0 && (y%100!=0 || y%400==0)) ? 1 : 0;
    if (m > 2 && leap) total_days++;
    for (int mo = 1; mo < m && mo <= 12; mo++)
        total_days += days_in_month[mo-1];
    total_days += d - 1;

    /* Can overflow uint32_t for extreme dates */
    uint32_t secs = (uint32_t)total_days * 86400u;
    secs += (uint32_t)t->hour * 3600u + (uint32_t)t->minute * 60u + (uint32_t)t->second;
    return secs;
}

void pax_unix_to_time(uint32_t ts, pax_time *out) {
    int days = (int)(ts / 86400);
    int rem = (int)(ts % 86400);
    out->hour = rem / 3600; rem %= 3600;
    out->minute = rem / 60; out->second = rem % 60;
    int y = 1970;
    while (1) {
        int yd = 365 + ((y%4==0 && (y%100!=0 || y%400==0)) ? 1 : 0);
        if (days < yd) break;
        days -= yd; y++;
    }
    out->year = y;
    int dm[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    if (y%4==0 && (y%100!=0 || y%400==0)) dm[1] = 29;
    out->month = 1;
    for (int i = 0; i < 12; i++) {
        if (days < dm[i]) break;
        days -= dm[i]; out->month++;
    }
    out->day = days + 1;
}

int pax_time_compare(const pax_time *a, const pax_time *b) {
    if (a->year != b->year) return a->year - b->year;
    if (a->month != b->month) return a->month - b->month;
    if (a->day != b->day) return a->day - b->day;
    if (a->hour != b->hour) return a->hour - b->hour;
    if (a->minute != b->minute) return a->minute - b->minute;
    return a->second - b->second;
}

int pax_time_format(uint32_t ts, char *buf, size_t buf_size) {
    pax_time t;
    pax_unix_to_time(ts, &t);
    int n = snprintf(buf, buf_size, "%04d-%02d-%02d %02d:%02d:%02d",
                     t.year, t.month, t.day, t.hour, t.minute, t.second);
    return n < (int)buf_size ? PAX_OK : PAX_ERR_OVERFLOW;
}
