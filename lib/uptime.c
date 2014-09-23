/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#include "common.h"
#include "uptime.h"

#include <stdio.h>


#define SEC_DAY     86400
#define SEC_HOUR    3600
#define SEC_MINUTE  60


static char fmt_buf[50];


uint32_t
uptime_get(void) {
    return CoGetOSTime() / S2ST(1);
}


const char *
uptime_format(void) {
    unsigned int t, n;
    char *ptr = fmt_buf;
    t = uptime_get();

    n = t / SEC_DAY;
    ptr += sprintf(ptr, "%u day%s, ", n, (n != 1) ? "s" : "");
    t -= (n * SEC_DAY);

    n = t / SEC_HOUR;
    ptr += sprintf(ptr, "%u hour%s, ", n, (n != 1) ? "s" : "");
    t -= (n * SEC_HOUR);

    n = t / SEC_MINUTE;
    ptr += sprintf(ptr, "%u minute%s, ", n, (n != 1) ? "s" : "");
    t -= (n * SEC_MINUTE);

    ptr += sprintf(ptr, "%u second%s", t, (t != 1) ? "s" : "");
    return fmt_buf;
}
