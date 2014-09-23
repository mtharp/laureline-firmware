/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#ifndef _PPSCAPTURE_H
#define _PPSCAPTURE_H

#define QUANT_LAGGING           1
#define QUANT_LEADING           2

void ppscapture_start(void);
uint64_t monotonic_getI(void);
uint64_t monotonic_now(void);
uint64_t monotonic_get_capture(void);
void monotonic_sleep_until(uint64_t mono_when);


#endif
