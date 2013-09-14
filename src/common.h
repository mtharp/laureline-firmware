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
#define SET_IRQ(save, pri)	do { (save) = __get_BASEPRI(); __set_BASEPRI((pri)); } while(0)
#define ADD_IRQ(save, pri)	do { (save) = __get_BASEPRI(); if ((pri) > (save)) __set_BASEPRI(pri); } while(0)
#define DISABLE_IRQ(save)	SET_IRQ(save, 0xFF)
#define RESTORE_IRQ(save)	__set_BASEPRI((save))
#define MS2ST(seconds)		(seconds * CFG_SYSTICK_FREQ / 1000)
#define S2ST(seconds)		(seconds * CFG_SYSTICK_FREQ)
#define SET_BITS(var, mask, value) \
	(var) = ((var) & ~(mask)) | ((value) & (mask))

#define GPIO_ON(pfx)		_PASTE2(pfx, _PAD)->BSRR = _PASTE2(pfx, _PIN);
#define GPIO_OFF(pfx)		_PASTE2(pfx, _PAD)->BRR  = _PASTE2(pfx, _PIN);

/* Highest priority (lowest number) */
#define THREAD_PRIO_VTIMER			10
#define THREAD_PRIO_MAIN			32
/* Lowest priority (highest number) */

/* Highest priority (lowest number) */
#define IRQ_PRIO_PPSCAPTURE			0x20
#define IRQ_PRIO_SYSTICK			0x80
#define IRQ_PRIO_I2C				0xC0
#define IRQ_PRIO_USART				0xC0
/* Lowest priority (highest number) */

#endif
