/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#ifndef _VTIMER_H
#define _VTIMER_H

#define NTP_SECOND			(1ULL<<32)
#define NTP_MASK_FRAC		((1ULL<<32)-1)
#define NTP_MASK_SECONDS	(((1ULL<<32)-1)<<32)
#define NTP_TO_FLOAT		4294967296.0 /* 2^32 */
#define NTP_TO_US			4295 /* 2^32 / 1e6 */

#define STEP_THRESH			20e-3
#define SETTLED_THRESH		5e-6

/* PPS LED is on for this long */
#define PPS_BLINK_TIME		0.050
/* PLL subroutine runs at this offset after the top of the second */
#define PLL_SUB_TIME		0.900


void vtimer_start(void);
uint64_t vtimer_now(void);
void vtimer_set_utc(uint16_t year, uint8_t month, uint8_t day,
		uint8_t hour, uint8_t minute, uint8_t second, uint8_t leap);
void vtimer_set_correction(double corr);


#endif
