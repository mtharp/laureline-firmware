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

#include "misc_macros.h"
#include "board.h"
#include "core_config.h"
#include "app_config.h"

#include "FreeRTOS.h"

#define TIMEOUT_NOBLOCK     0
#define TIMEOUT_FOREVER     -1

#define EERR_OK             0
#define EERR_TIMEOUT        -1
#define EERR_FAULT          -2
#define EERR_INVALID        -3
#define EERR_NACK           -4
#define EERR_CRCFAIL        -5
#define EERR_AGAIN          -6

#define GPIO_ON(pfx)        _PASTE2(pfx, _PAD)->BSRR = _PASTE2(pfx, _PIN);
#define GPIO_OFF(pfx)       _PASTE2(pfx, _PAD)->BRR  = _PASTE2(pfx, _PIN);

#endif
