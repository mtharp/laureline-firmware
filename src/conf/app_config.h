/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#ifndef _APP_CONFIG_H
#define _APP_CONFIG_H

#define USE_I2C1                1
#define USE_I2C2                0
#define USE_SERIAL_USART1       1
#define USE_SERIAL_USART2       0
#define USE_SERIAL_USART2       0
#define USE_SERIAL_UART4        1
#define USE_SERIAL_UART5        1
#define USE_SPI1                0
#define USE_SPI3                0

/* Highest priority (highest number) */
#define THREAD_PRIO_VTIMER      4
#define THREAD_PRIO_MAIN        3
#define THREAD_PRIO_TCPIP       2
#define THREAD_PRIO_NTPCLIENT   1
/* Lowest priority (lowest number) */

/* Highest priority (lowest number) */
#define IRQ_PRIO_PPSCAPTURE     2
#define IRQ_PRIO_ETH            4
#define IRQ_PRIO_SYSTICK        8
#define IRQ_PRIO_I2C            12
#define IRQ_PRIO_SPI            12
#define IRQ_PRIO_USART          12
/* Lowest priority (highest number) */

#define MSP_STACK_SIZE          512
#define MAIN_STACK_SIZE         512
#define NTPCLIENT_STACK_SIZE    512
#define TCPIP_STACK_SIZE        512
#define VTIMER_STACK_SIZE       512

#endif
