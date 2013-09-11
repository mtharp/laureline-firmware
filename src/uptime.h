/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#ifndef _UPTIME_H
#define _UPTIME_H

#include <stdint.h>

void uptime_refresh(void);
uint32_t uptime_get(void);
const char *uptime_format(void);


#endif
