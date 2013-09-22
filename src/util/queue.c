/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#include "common.h"
#include "util/queue.h"


void
outqueue_init(queue_t *q, uint8_t *buf, uint8_t size) {
	ASSERT((q->flag = CoCreateFlag(1, 0)) != E_CREATE_FAIL);
	q->p_bot = q->p_read = q->p_write = buf;
	q->p_top = buf + size;
	/* In an output queue, count is the number of bytes occupied */
	q->size = size;
	q->count = q->wakeup = 0;
	q->cb_func = NULL;
	q->cb_arg = NULL;
}


void
outqueue_cb(queue_t *q, cb_func_t cb_func, void *arg) {
	q->cb_func = cb_func;
	q->cb_arg = arg;
}


StatusType
outqueue_put(queue_t *q, const uint8_t *value, uint16_t size, uint32_t timeout) {
	StatusType rc;
	if (timeout) {
		/* relative time -> absolute deadline */
		timeout += CoGetOSTime();
	}
	DISABLE_IRQ();
	while (size) {
		while (q->count == q->size) {
			/* Wake this thread up when the buffer has room for all remaining
			 * bytes, or is empty. */
			if (size > q->size) {
				q->wakeup = 0;
			} else {
				q->wakeup = q->size - size;
			}
			ENABLE_IRQ();
			if (q->cb_func) {
				q->cb_func(q->cb_arg);
			}
			rc = CoWaitForSingleFlag(q->flag,
					timeout ? (timeout - CoGetOSTime()) : 0);
			if (rc != E_OK) {
				return rc;
			}
			DISABLE_IRQ();
		}
		size--;
		q->count++;
		*q->p_write++ = *value++;
		if (q->p_write == q->p_top) {
			q->p_write = q->p_bot;
		}
	}
	ENABLE_IRQ();
	if (q->cb_func) {
		q->cb_func(q->cb_arg);
	}
	return E_OK;
}


int16_t
outqueue_getI(queue_t *q) {
	uint16_t ret;
	if (q->count == 0) {
		return -1;
	}
	ret = *q->p_read++;
	if (q->p_read == q->p_top) {
		q->p_read = q->p_bot;
	}
	if (--q->count <= q->wakeup) {
		isr_SetFlag(q->flag);
	}
	return ret;
}
