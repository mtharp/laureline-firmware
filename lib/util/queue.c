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
queue_init(queue_t *q, uint8_t *buf, uint8_t size) {
	ASSERT((q->flag = CoCreateFlag(1, 0)) != E_CREATE_FAIL);
	q->p_bot = q->p_read = q->p_write = buf;
	q->p_top = buf + size;
	q->size = q->wakeup = size;
	q->count = 0;
	q->cb_func = NULL;
	q->cb_arg = NULL;
}


void
queue_cb(queue_t *q, cb_func_t cb_func, void *arg) {
	q->cb_func = cb_func;
	q->cb_arg = arg;
}


int16_t
outqueue_put(queue_t *q, const uint8_t *value, uint16_t size, int timeout) {
	StatusType rc;
	uint64_t deadline = 0;
	if (timeout > TIMEOUT_NOBLOCK) {
		deadline = timeout + CoGetOSTime();
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
			if (timeout == TIMEOUT_NOBLOCK) {
				return EERR_TIMEOUT;
			}
			rc = CoWaitForSingleFlag(q->flag,
					timeout == TIMEOUT_FOREVER ? 0 : (CoGetOSTime() - deadline));
			if (rc != E_OK) {
				return EERR_TIMEOUT;
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
	return EERR_OK;
}


void
outqueue_drain(queue_t *q) {
	DISABLE_IRQ();
	while (1) {
		if (q->count == 0) {
			break;
		}
		ENABLE_IRQ();
		__ISB();
		DISABLE_IRQ();
	}
	ENABLE_IRQ();
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


void
inqueue_putI(queue_t *q, uint8_t value) {
	if (q->count == q->size) {
		return;
	}
	*q->p_write++ = value;
	if (q->p_write == q->p_top) {
		q->p_write = q->p_bot;
	}
	if (++q->count >= q->wakeup) {
		isr_SetFlag(q->flag);
	}
}


void
inqueue_flushI(queue_t *q) {
	if (q->count > 0) {
		isr_SetFlag(q->flag);
	}
}


int16_t
inqueue_get(queue_t *q, int timeout) {
	StatusType rc;
	uint64_t deadline = 0;
	uint8_t value;
	if (timeout > TIMEOUT_NOBLOCK) {
		deadline = timeout + CoGetOSTime();
	}
	DISABLE_IRQ();
	while (q->count == 0) {
		ENABLE_IRQ();
		if (timeout == TIMEOUT_NOBLOCK) {
			return EERR_TIMEOUT;
		}
		rc = CoWaitForSingleFlag(q->flag,
				timeout == TIMEOUT_FOREVER ? 0 : (CoGetOSTime() - deadline));
		if (rc != E_OK) {
			return EERR_TIMEOUT;
		}
		DISABLE_IRQ();
	}
	q->count--;
	value = *q->p_read++;
	if (q->p_read == q->p_top) {
		q->p_read = q->p_bot;
	}
	ENABLE_IRQ();
	return value;
}
