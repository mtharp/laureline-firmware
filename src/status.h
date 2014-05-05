/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */


#ifndef _STATUS_H
#define _STATUS_H

#define STATUS_PPS_OK				0x01
#define STATUS_TOD_OK				0x02
#define STATUS_PLL_OK				0x04
#define STATUS_USED_QUANT			0x08

/* pll is settled */
#define STATUS_SETTLED (STATUS_PPS_OK | STATUS_PLL_OK)
/* all data from GPS is ok */
#define STATUS_VALID (STATUS_PPS_OK | STATUS_TOD_OK)
/* ready to serve ntp */
#define STATUS_READY (STATUS_PLL_OK | STATUS_TOD_OK)

extern uint16_t status_flags;

void set_status(uint16_t mask);
void clear_status(uint16_t mask);

#endif
