/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#ifndef _COOS_PLAT_H
#define _COOS_PLAT_H

extern uint32_t _irq_disabled;
#define DISABLE_IRQ()		do { __disable_irq(); _irq_disabled++; } while(0)
#define ENABLE_IRQ()		do { if (--_irq_disabled == 0) { __enable_irq(); } } while(0)
#define SAVE_ENABLE_IRQ(save) do { (save) = _irq_disabled; _irq_disabled = 0; __enable_irq(); } while(0)
#define RESTORE_DISABLE_IRQ(save) do { __disable_irq(); if ((_irq_disabled = (save)) == 0) { __enable_irq(); } } while(0)

#define HALT()				while(1) {}
#define ASSERT(x)			if (!(x)) { HALT(); }
#define MS2ST(msec)			((((msec - 1L) * CFG_SYSTICK_FREQ) / 1000L) + 1L)
#define S2ST(seconds)		(seconds * CFG_SYSTICK_FREQ)

#endif
