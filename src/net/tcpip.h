/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#ifndef _TCPIP_H
#define _TCPIP_H

#include "lwip/netif.h"

#define IP_DIGITS(addr) (addr) & 0xff, (addr >> 8) & 0xff, (addr >> 16) & 0xff, (addr >> 24) & 0xff

extern struct netif thisif;

void tcpip_start(void);

#endif
