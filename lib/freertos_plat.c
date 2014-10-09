/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#include "common.h"
#include "task.h"

static uint64_t milliseconds;


void
vApplicationIdleHook(void) {
    __WFI();
}


void
vApplicationTickHook(void) {
    milliseconds += 1000 / configTICK_RATE_HZ;
}


void
vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    HALT();
}


uint64_t
milliseconds_get(void) {
    uint64_t ret;
    taskENTER_CRITICAL();
    ret = milliseconds;
    taskEXIT_CRITICAL();
    return ret;
}
