/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#ifndef _UBLOX_H
#define _UBLOX_H

#include "stm32/serial.h"

uint8_t ublox_feed(uint8_t data);
void ublox_configure(void);

/* applicable to both NAV-TIMEUTC and NAV-TIMEGPS */
#define TIMEUTC_VALIDTOW        0x01
#define TIMEUTC_VALIDWKN        0x02
#define TIMEUTC_VALIDUTC        0x04

#pragma pack(push,1)

typedef struct {
    uint16_t msgid, length;
    uint32_t iTOW;
    int32_t fTOW;
    int16_t week;
    int8_t leapS;
    uint8_t valid;
    uint32_t tAcc;
} nav_timegps_t;

typedef struct {
    uint16_t msgid, length;
    uint32_t iTOW;
    uint32_t tAcc;
    int32_t nano;
    uint16_t year;
    uint8_t month, day;
    uint8_t hour, min, sec;
    uint8_t valid;
} nav_timeutc_t;

#pragma pack(pop)

#endif
