#
# Copyright (c) Michael Tharp <gxti@partiallystapled.com>
#
# This file is distributed under the terms of the MIT License.
# See the LICENSE file at the top of this tree, or if it is missing a copy can
# be found at http://opensource.org/licenses/MIT
#

PROJECT = laureline
BOARD = boards/v5
USE_LINK_GC = yes

SRCS = $(wildcard src/*.c)
SRCS += $(wildcard ports/*.c ports/*.s)
SRCS += $(wildcard CoOS/kernel/*.c)
SRCS += $(wildcard CoOS/portable/*.c )
SRCS += $(wildcard CoOS/portable/GCC/*.c)
LDSCRIPT = ports/STM32F107xB.ld

PATH := /opt/tnt/bin:$(PATH)
override GDB = /opt/gat/bin/arm-none-eabi-gdb
MCFLAGS = -mcpu=cortex-m3 -mthumb
CFLAGS = \
	-Isrc \
	-Iports \
	-ICoOS/kernel \
	-ICoOS/portable \
	\
	$(MCFLAGS) \
	-falign-functions=16 \
	-Wall -Wextra -Wstrict-prototypes \
	-Wno-unused-parameter \
	-Wno-main \
	-ggdb3
ifdef DEBUG
CFLAGS += -O0
else
CFLAGS += -O3 -fomit-frame-pointer
endif
ASFLAGS = $(MCFLAGS)
LDFLAGS = $(MCFLAGS)
LDLIBS = -lm

include emk/rules.mk
