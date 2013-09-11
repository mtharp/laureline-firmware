/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#include "common.h"
#include "init.h"
#include "serial.h"

#define TASK0_PRI 10
#define TASK0_STACK 100
OS_STK task0stack[TASK0_STACK];
OS_TID task0id;


void
task0(void *pdata) {
	uint8_t data;
	while (1) {
		data = serial_getc(&Serial4);
		serial_putc(&Serial1, data);
	}
}


void
main(void) {
	CoInitOS();
	setup_clocks(ONBOARD_CLOCK);
	serial_start(&Serial1, USART1, 115200);
	serial_start(&Serial4, UART4, 57600);
	task0id = CoCreateTask(task0, NULL, TASK0_PRI,
			&task0stack[TASK0_STACK-1], TASK0_STACK);
	CoStartOS();
	while (1) {}
}
