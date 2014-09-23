/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#include <stdint.h>
#include "util/parse.h"


uint8_t
parse_hex(char dat) {
    if (dat >= '0' && dat <= '9') {
        dat -= '0';
    } else if (dat >= 'A' && dat <= 'F') {
        dat -= 'A' - 10;
    } else if (dat >= 'a' && dat <= 'f') {
        dat -= 'a' - 10;
    } else {
        return 0;
    }
    return dat;
}


uint8_t
atoi_2dig(const char *str) {
    /* 2 decimal digits to integer */
    uint8_t ret;
    if (str[0] < '0' || str[0] > '9' || str[1] < '0' || str[1] > '9') {
        return 0;
    }
    ret = str[1] - '0';
    ret += (str[0] - '0') * 10;
    return ret;
}


uint32_t
atoi_decimal(const char *str) {
    /* Copy decimal digits until the first period or end of string */
    uint32_t ret = 0;
    while (*str && *str != '.') {
        if (*str < '0' || *str > '9') {
            return 0;
        }
        ret = (ret * 10) + (*str - '0');
        str++;
    }
    return ret;
}


char *
strtok_s(char *str, const char delim) {
    /* Like strtok, but with a single delimiter and no malloc */
    static char *last;
    char *tmp;
    if (str) {
        last = str;
    } else if (last) {
        str = last;
    } else {
        return 0;
    }
    while (*str) {
        if (*str == delim) {
            *str++ = 0;
            tmp = last;
            last = str;
            return tmp;
        }
        str++;
    }
    tmp = last;
    last = 0;
    return tmp;
}
