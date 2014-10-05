/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#pragma once
#include "common.h"

typedef void  (*irq_vector_t)(void);

extern irq_vector_t _isr_vector[];

#define BootRAM 0xF1E0F85F

