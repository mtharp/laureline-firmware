/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#ifndef _MISC_MACROS_H
#define _MISC_MACROS_H

/* stupid trick to work around cpp macro handling */
#define _PASTE(x,y) x##y
#define _PASTE2(x,y) _PASTE(x,y)

#define SET_BITS(var, mask, value) \
    (var) = ((var) & ~(mask)) | ((value) & (mask))

#define MAX(a, b)  ((a) > (b) ? (a) : (b))
#define MIN(a, b)  ((a) < (b) ? (a) : (b))

#define HALT()              while(1) {}
#define ASSERT(x)           if (!(x)) { HALT(); }

#endif
