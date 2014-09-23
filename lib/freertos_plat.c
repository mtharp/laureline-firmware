/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#include "common.h"
#include "task.h"

uint64_t ticks_wide;


void
vApplicationIdleHook(void) {
    __WFI();
}


void
vApplicationTickHook(void) {
    ticks_wide++;
}


void
vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    HALT();
}


uint64_t
xGetTaskTickCountLong(void) {
    uint64_t ret;
    taskENTER_CRITICAL();
    ret = ticks_wide;
    taskEXIT_CRITICAL();
    return ret;
}
