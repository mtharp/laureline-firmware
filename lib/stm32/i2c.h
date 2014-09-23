/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#ifndef _I2C_H
#define _I2C_H

#include "common.h"
#include "semphr.h"


typedef struct {
    I2C_TypeDef *dev;
    SemaphoreHandle_t mutex;
    SemaphoreHandle_t sem;
    uint8_t *buf;
    uint8_t addr_dir;
    uint8_t count;
    uint8_t index;
    uint8_t error;
} i2c_t;

#define I2C_T_INITIALIZER \
    /* dev, */ NULL, NULL, NULL, 0, 0, 0, 0


#if USE_I2C1
extern i2c_t I2C1_Dev;
#endif
#if USE_I2C2
extern i2c_t I2C2_Dev;
#endif

void i2c_start(i2c_t *i2c);
void i2c_stop(i2c_t *i2c);
int16_t i2c_transact(i2c_t *i2c, uint8_t addr_dir, uint8_t *buf, size_t count);

#endif
