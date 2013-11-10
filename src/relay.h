/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#ifndef _RELAY_H
#define _RELAY_H

void relay_server_start(uint16_t port);

void relay_push(uint8_t value);
void relay_flush(void);

#endif
