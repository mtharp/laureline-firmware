/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#ifndef _NTPNS_H
#define _NTPNS_H

#define ABLE_STARTUP            (1 << 0)
#define ABLE_PLL_UNLOCKED       (1 << 1)
#define ABLE_NO_SOURCE          (1 << 2)
#define ABLE_SOURCE_OFFSET      (1 << 3)

extern unsigned sys_able;

void init_pllmath(void);

#endif
