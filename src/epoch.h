/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#ifndef _EPOCH_H
#define _EPOCH_H

#include <time.h>

/* Days from 1-1-1 to 1970-1-1 */
#define UNIX_EPOCH_ORDINAL 719163
/* Days from 1-1-1 to 1900-1-1 */
#define NTP_EPOCH_ORDINAL 693596

uint64_t datetime_to_epoch(uint16_t year, uint8_t month, uint8_t day,
		uint8_t hour, uint8_t minute, uint8_t second);
void epoch_to_datetime(uint64_t time, struct tm *tm);

#endif
