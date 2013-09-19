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
	q->count = size;
}


StatusType
outqueue_put(queue_t *q, uint8_t value, uint32_t timeout) {
	StatusType rc;
	if (timeout) {
		timeout += CoGetOSTime();
	}
	while (1) {
		DISABLE_IRQ();
		if (q->count > 0) {
			break;
		}
		ENABLE_IRQ();
		rc = CoWaitForSingleFlag(q->flag,
				timeout ? (timeout - CoGetOSTime()) : 0);
		if (rc != E_OK) {
			return rc;
		}
	}
	q->count--;
	*q->p_write++ = value;
	if (q->p_write == q->p_top) {
		q->p_write = q->p_bot;
	}
	ENABLE_IRQ();
	return E_OK;
}


uint16_t
outqueue_getI(queue_t *q) {
	uint16_t ret;
	if (q->p_write == q->p_read && q->count != 0) {
		return -1;
	}
	q->count++;
	ret = *q->p_read++;
	if (q->p_read == q->p_top) {
		q->p_read = q->p_bot;
	}
	isr_SetFlag(q->flag);
	return ret;
}
