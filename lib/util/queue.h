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

typedef void (*cb_func_t)(void *arg);

typedef struct {
	OS_FlagID flag;
	uint8_t size;
	volatile uint8_t count, wakeup;
	uint8_t *p_bot, *p_top, *p_write, *p_read;
	cb_func_t cb_func;
	void *cb_arg;
} queue_t;

void queue_init(queue_t *q, uint8_t *buf, uint8_t size);
void queue_cb(queue_t *q, cb_func_t cb_func, void *arg);

int16_t outqueue_put(queue_t *q, const uint8_t *value, uint16_t size, int timeout);
int16_t outqueue_getI(queue_t *q);

int16_t inqueue_get(queue_t *q, int timeout);
void inqueue_putI(queue_t *q, uint8_t value);
void inqueue_flushI(queue_t *q);

#endif
