/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#ifndef _NTPSERVER_H
#define _NTPSERVER_H

#define NTP_PORT				123

#define LEAP_MASK				0xC0
#define LEAP_NONE				0x0
#define LEAP_INSERT				0x1
#define LEAP_DELETE				0x2
#define LEAP_UNKNOWN			0x3
#define VN_MASK					0x38
#define VN_4					(4 << 3)
#define MODE_MASK				0x07
#define MODE_CLIENT				0x3
#define MODE_SERVER				0x4

void ntp_server_start(void);

#endif
