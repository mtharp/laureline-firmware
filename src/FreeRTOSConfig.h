/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include "app_config.h"

extern sysfreq_t system_frequency;
#ifdef BOOTLOADER
#define configUSE_IDLE_HOOK             0
#define configUSE_QUEUE_SETS            0
#define configUSE_TICK_HOOK             0
#define configCHECK_FOR_STACK_OVERFLOW  0
#else
#define configUSE_IDLE_HOOK             1
#define configUSE_QUEUE_SETS            1
#define configUSE_TICK_HOOK             1
#define configCHECK_FOR_STACK_OVERFLOW  2
#endif

#define configUSE_16_BIT_TICKS          0
#define configUSE_ALTERNATIVE_API       0
#define configUSE_CO_ROUTINES           0
#define configUSE_RECURSIVE_MUTEXES     0
#define configUSE_TIME_SLICING          0
#define configUSE_TIMERS                0
#define configUSE_TRACE_FACILITY        0

#define configUSE_COUNTING_SEMAPHORES   1
#define configUSE_MUTEXES               1
#define configUSE_PREEMPTION            1

#define configASSERT(x)                 if (!(x)) { while(1) {} }
#define configCPU_CLOCK_HZ              system_frequency
#define configTICK_RATE_HZ              ( ( TickType_t ) 100 )
#define configMAX_PRIORITIES            5
#define configMINIMAL_STACK_SIZE        128
#define configMAX_TASK_NAME_LEN         8
#define configIDLE_SHOULD_YIELD         0
#define configQUEUE_REGISTRY_SIZE       0
#define configGENERATE_RUN_TIME_STATS   0
#define configTIMER_TASK_PRIORITY       (configMAX_PRIORITIES - 1)
#define configTIMER_QUEUE_LENGTH        2
#define configTIMER_TASK_STACK_DEPTH    128

#define INCLUDE_vTaskCleanUpResources   0
#define INCLUDE_vTaskDelete             0
#define INCLUDE_xTimerPendFunctionCall  0
#define INCLUDE_vTaskDelayUntil         0
#define INCLUDE_uxTaskPriorityGet       1
#define INCLUDE_vTaskDelay              1
#define INCLUDE_vTaskPrioritySet        1
#define INCLUDE_vTaskSuspend            1

/* Use lowest possible interrupt priority for the scheduler */
#define configKERNEL_INTERRUPT_PRIORITY 0xff
/* Interrupts with higher priority than this (lower number) will preempt the
 * scheduler, but cannot use any RTOS functions. */
#define configMAX_SYSCALL_INTERRUPT_PRIORITY 0x10


/* This is the value being used as per the ST library which permits 16
priority values, 0 to 15.  This must correspond to the
configKERNEL_INTERRUPT_PRIORITY setting.  Here 15 corresponds to the lowest
NVIC value of 255. */
#define configLIBRARY_KERNEL_INTERRUPT_PRIORITY 15

#define vPortSVCHandler SVC_Handler
#define xPortPendSVHandler PendSV_Handler
#define xPortSysTickHandler SysTick_Handler

#endif /* FREERTOS_CONFIG_H */
