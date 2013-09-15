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

/* stupid trick to work around cpp macro handling */
#define _PASTE(x,y) x##y
#define _PASTE2(x,y) _PASTE(x,y)

#define HALT()				while(1) {}
#define ASSERT(x)			if (!(x)) { HALT(); }
#define MS2ST(msec)			((((msec - 1L) * CFG_SYSTICK_FREQ) / 1000L) + 1L)
#define S2ST(seconds)		(seconds * CFG_SYSTICK_FREQ)
#define SET_BITS(var, mask, value) \
	(var) = ((var) & ~(mask)) | ((value) & (mask))

extern uint32_t _irq_disabled;
#define DISABLE_IRQ()		do { __disable_irq(); _irq_disabled++; } while(0)
#define ENABLE_IRQ()		do { if (--_irq_disabled == 0) { __enable_irq(); } } while(0)

#define GPIO_ON(pfx)		_PASTE2(pfx, _PAD)->BSRR = _PASTE2(pfx, _PIN);
#define GPIO_OFF(pfx)		_PASTE2(pfx, _PAD)->BRR  = _PASTE2(pfx, _PIN);

/* Highest priority (lowest number) */
#define THREAD_PRIO_VTIMER			10
#define THREAD_PRIO_TCPIP			20
#define THREAD_PRIO_MAIN			30
/* Lowest priority (highest number) */

/* Highest priority (lowest number) */
#define IRQ_PRIO_PPSCAPTURE			0x20
#define IRQ_PRIO_ETH				0x40
#define IRQ_PRIO_SYSTICK			0x80
#define IRQ_PRIO_I2C				0xC0
#define IRQ_PRIO_USART				0xC0
/* Lowest priority (highest number) */

#endif
