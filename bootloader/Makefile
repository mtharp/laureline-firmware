#
# Copyright (c) Michael Tharp <gxti@partiallystapled.com>
#
# This file is distributed under the terms of the MIT License.
# See the LICENSE file at the top of this tree, or if it is missing a copy can
# be found at http://opensource.org/licenses/MIT
#

PROJECT = bootloader
BOARD = boards/v5
FLASH_SIZE = 256k

TOP = ..
SRCS = $(wildcard src/*.c)
SRCS += $(TOP)/fatfs/ff.c
SRCS += $(TOP)/lib/coos_plat.c
SRCS += $(TOP)/lib/crc7.c
SRCS += $(TOP)/lib/fatfs/mmc_diskio.c
SRCS += $(TOP)/lib/hardfault.c
SRCS += $(TOP)/lib/ihex.c
SRCS += $(TOP)/lib/stm32/dma.c
SRCS += $(TOP)/lib/stm32/flash.c
SRCS += $(TOP)/lib/stm32/i2c.c
SRCS += $(TOP)/lib/stm32/mmc.c
SRCS += $(TOP)/lib/stm32/serial.c
SRCS += $(TOP)/lib/stm32/spi.c
SRCS += $(TOP)/lib/util/queue.c
SRCS += $(TOP)/ports/core_cm3.c
SRCS += $(TOP)/ports/crt0.c
SRCS += $(TOP)/ports/stubs.c
SRCS += $(TOP)/ports/vectors.c
SRCS += $(TOP)/src/board.c
SRCS += $(TOP)/src/init.c
SRCS += $(wildcard $(TOP)/CoOS/kernel/*.c)
SRCS += $(TOP)/CoOS/portable/arch.c
SRCS += $(TOP)/CoOS/portable/GCC/port.c
LDSCRIPT = ports/STM32F107xB.ld

PATH := /opt/tnt-20130915/bin:$(PATH)
INCLUDES = \
	-Isrc \
	-I$(TOP)/src \
	-I$(TOP)/lib \
	-I$(TOP)/lib/lwip \
	-I$(TOP)/ports \
	-I$(TOP)/CoOS/kernel \
	-I$(TOP)/CoOS/portable \
	-I$(TOP)/fatfs

CFLAGS = \
	$(INCLUDES) \
	-Wall -Wextra -Wstrict-prototypes \
	-Wno-unused-parameter \
	-Wno-main \
	-Wno-address

include $(TOP)/emk/rules.mk

LD := $(PYTHON) $(TOP)/util/resize_ld.py $(FLASH_SIZE) $(LD)
