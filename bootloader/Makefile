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
SRCS += ../src/board.c
SRCS += ../src/common.c
SRCS += ../src/hardfault.c
SRCS += ../src/init.c
SRCS += $(wildcard $(TOP)/src/periph/*.c)
SRCS += $(wildcard $(TOP)/src/util/*.c)
SRCS += $(wildcard $(TOP)/ports/*.c)
SRCS += $(wildcard $(TOP)/CoOS/kernel/*.c)
SRCS += $(wildcard $(TOP)/CoOS/portable/*.c )
SRCS += $(wildcard $(TOP)/CoOS/portable/GCC/*.c)
LDSCRIPT = ports/STM32F107xB.ld

PATH := /opt/tnt-20130915/bin:$(PATH)
INCLUDES = \
	-Isrc \
	-Iports \
	-I$(TOP)/src \
	-I$(TOP)/ports \
	-I$(TOP)/CoOS/kernel \
	-I$(TOP)/CoOS/portable

CFLAGS = \
	$(INCLUDES) \
	-Wall -Wextra -Wstrict-prototypes \
	-Wno-unused-parameter \
	-Wno-main \
	-Wno-address

SRCS += $(TOP)/src/crc7.c $(TOP)/fatfs/ff.c $(TOP)/src/fatfs/diskio.c
INCLUDES += -I$(TOP)/fatfs -I$(TOP)/src/fatfs

include $(TOP)/emk/rules.mk
