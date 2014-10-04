/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#ifndef _INIT_H
#define _INIT_H

#define GPIO_MODE_INPUT         0b0000
#define GPIO_MODE_10MHZ         0b0001
#define GPIO_MODE_2MHZ          0b0010
#define GPIO_MODE_50MHZ         0b0011

#define GPIO_INPUT_ANALOG       0b0000
#define GPIO_INPUT_FLOATING     0b0100
#define GPIO_INPUT_PUPD         0b1000
#define GPIO_OUTPUT_PP          0b0000
#define GPIO_OUTPUT_OD          0b0100
#define GPIO_AFIO_PP            0b1000
#define GPIO_AFIO_OD            0b1100


extern sysfreq_t system_frequency;

typedef struct {
    uint8_t flags;
    uint8_t value;
} gpio_cfg_t;

void setup_clocks(double hse_freq);
void setup_hsi(void);
void setup_gpio(GPIO_TypeDef *bank, int pin, int flags, int value);
void setup_bank(GPIO_TypeDef *bank, const gpio_cfg_t *pins);

#endif
