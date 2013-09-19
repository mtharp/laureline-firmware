/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#ifndef _UTIL_QUEUE_H
#define _UTIL_QUEUE_H

#include "common.h"

typedef struct {
	OS_FlagID flag;
	volatile uint8_t count;
	uint8_t *p_bot, *p_top, *p_write, *p_read;
} queue_t;

void outqueue_init(queue_t *q, uint8_t *buf, uint8_t size);
StatusType outqueue_put(queue_t *q, uint8_t value, uint32_t timeout);
uint16_t outqueue_getI(queue_t *q);

#endif
