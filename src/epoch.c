/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */
/*
 * Parts of this file Copyright (c) 2001-2010 Python Software Foundation; All Rights Reserved
 */

#include "common.h"

/* Days from 1-1-1 to 1900-1-1 */
#define NTP_EPOCH_ORDINAL 693596

static const uint16_t _days_before_month[] = {
		0, /* unused; this vector uses 1-based indexing */
		0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
};

/* year -> 1 if leap year, else 0. */
static uint8_t
is_leap(uint16_t year)
{
    return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}

/* year, month -> number of days in year preceeding first day of month */
static uint16_t
days_before_month(uint16_t year, uint8_t month)
{
    uint16_t days;
    days = _days_before_month[month];
    if (month > 2 && is_leap(year))
        ++days;
    return days;
}

/* year -> number of days before January 1st of year.  Remember that we
 * start with year 1, so days_before_year(1) == 0.
 */
static uint32_t
days_before_year(uint16_t year)
{
    year--;
    return year*365 + year/4 - year/100 + year/400;
}

uint32_t
datetime_to_ntp(uint16_t year, uint8_t month, uint8_t day,
		uint8_t hour, uint8_t minute, uint8_t second)
{
	uint32_t ordinal = days_before_year(year) + days_before_month(year, month) + day - NTP_EPOCH_ORDINAL;
	return ordinal * 86400 + (hour * 3600 + minute * 60 + second);
}


