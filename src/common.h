/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */


#ifndef _COMMON_H
#define _COMMON_H

#include <stddef.h>
#include <stdint.h>

#include "board.h"

#include <CoOS.h>
#include <stm32f10x.h>

#define HALT() while(1) {}
#define MS2ST(seconds) (seconds * CFG_SYSTICK_FREQ / 1000)
#define S2ST(seconds) (seconds * CFG_SYSTICK_FREQ)
#define SET_BITS(var, mask, value) \
	(var) = ((var) & ~(mask)) | ((value) & (mask))

#endif
