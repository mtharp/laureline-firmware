/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#ifndef _UTIL_PARSE_H
#define _UTIL_PARSE_H

uint8_t parse_hex(char dat);
uint8_t atoi_2dig(const char *str);
uint32_t atoi_decimal(const char *str);
char *strtok_s(char *str, const char delim);

#endif
