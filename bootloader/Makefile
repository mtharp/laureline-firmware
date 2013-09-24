#
# Copyright (c) Michael Tharp <gxti@partiallystapled.com>
#
# This file is distributed under the terms of the MIT License.
# See the LICENSE file at the top of this tree, or if it is missing a copy can
# be found at http://opensource.org/licenses/MIT
#

PROJECT = bootloader
BOARD = boards/v5

TOP = ..
SRCS = $(wildcard src/*.c)
SRCS += \
	../src/board.c \
	../src/common.c \
	../src/init.c \
	../src/serial.c
SRCS += $(wildcard $(TOP)/src/util/*.c)
SRCS += $(wildcard $(TOP)/ports/*.c)
SRCS += $(wildcard $(TOP)/CoOS/kernel/*.c)
SRCS += $(wildcard $(TOP)/CoOS/portable/*.c )
SRCS += $(wildcard $(TOP)/CoOS/portable/GCC/*.c)
LDSCRIPT = ports/STM32F107xB.ld

PATH := /opt/tnt-20130915/bin:$(PATH)
CFLAGS = \
	-Isrc \
	-Iports \
	-I$(TOP)/src \
	-I$(TOP)/ports \
	-I$(TOP)/CoOS/kernel \
	-I$(TOP)/CoOS/portable \
	\
	-Wall -Wextra -Wstrict-prototypes \
	-Wno-unused-parameter \
	-Wno-main \
	-Wno-address
LDLIBS = -lm

include $(TOP)/emk/rules.mk
