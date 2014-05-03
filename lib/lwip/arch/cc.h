/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#ifndef __CC_H__
#define __CC_H__

#include "common.h"
#include "cmdline.h"
#include <stdlib.h>

typedef uint8_t			u8_t;
typedef int8_t			s8_t;
typedef uint16_t		u16_t;
typedef int16_t			s16_t;
typedef uint32_t		u32_t;
typedef int32_t			s32_t;
typedef uint32_t		mem_ptr_t;

#define U16_F "%hu"
#define U32_F "%u"

#define LWIP_PLATFORM_DIAG(x)
#define LWIP_PLATFORM_ASSERT(x) { HALT(); }
#define LWIP_RAND() rand()
#define SRAND(x) srand(x)

#define BYTE_ORDER LITTLE_ENDIAN
#define LWIP_PROVIDE_ERRNO

#endif /* __CC_H__ */
