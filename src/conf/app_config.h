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

//#define MMCSPI (&SPI3_Dev)

/* Highest priority (lowest number) */
#define THREAD_PRIO_VTIMER          10
#define THREAD_PRIO_TCPIP           20
#define THREAD_PRIO_MAIN            30
#define THREAD_PRIO_NTPCLIENT       35
/* Lowest priority (highest number) */

/* Highest priority (lowest number) */
#define IRQ_PRIO_PPSCAPTURE         0x20
#define IRQ_PRIO_ETH                0x40
#define IRQ_PRIO_SYSTICK            0x80
#define IRQ_PRIO_I2C                0xC0
#define IRQ_PRIO_SPI                0xC0
#define IRQ_PRIO_USART              0xC0
/* Lowest priority (highest number) */

#define MSP_STACK_SIZE 512

#endif
