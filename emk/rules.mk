#
# Copyright (c) Michael Tharp <gxti@partiallystapled.com>
#
# This file is distributed under the terms of the MIT License.
# See the LICENSE file at the top of this tree, or if it is missing a copy can
# be found at http://opensource.org/licenses/MIT
#


BUILD ?= build
PROJECT ?= target

C_SRCS = $(filter %.c,$(SRCS))
S_SRCS = $(filter %.s,$(SRCS))
OUT = $(BUILD)/$(PROJECT)
BMP_SCRIPT = $(BUILD)/bmp.gdb

TRGT := arm-none-eabi-
CC   := $(TRGT)gcc
CPPC := $(TRGT)g++
LD   := $(TRGT)gcc -nostartfiles
CP   := $(TRGT)objcopy
AS   := $(TRGT)gcc -x assembler-with-cpp
OD   := $(TRGT)objdump
HEX  := $(CP) -O ihex
BIN  := $(CP) -O binary
GDB  := $(TRGT)gdb
PYTHON := python

CFLAGS += -mno-thumb-interwork -DTHUMB_NO_INTERWORKING
LDFLAGS += -T$(LDSCRIPT) -Wl,-Map=$(OUT).map,--cref,--no-warn-mismatch
ifeq ($(USE_LINK_GC),yes)
CFLAGS += -ffunction-sections -fdata-sections -fno-common
LDFLAGS += -Wl,--gc-sections
endif

CFLAGS += -MD -MP -MF $@.d

# Targets
all: $(OUT).elf $(OUT).bin $(OUT).hex $(OUT).lst

clean:
	rm -rf $(BUILD)

install: all
ifdef BMP
	@echo -e "tar ext $(BMP)\nmon swdp_scan\natt 1" >$(BMP_SCRIPT)
	$(GDB) $(OUT).elf -x $(BMP_SCRIPT) --batch -ex load -ex kill
else
	@echo usage: make install BMP=/dev/ttyACMx
	@exit 1
endif


# Rules
OBJS =
define c_template
OBJS += $(1)
$(1): $(2)
	@mkdir -p $(dir $(1))
	$$(CC) $$(CFLAGS) $$< -c -o $$@
endef
$(foreach src,$(C_SRCS),$(eval $(call c_template,$(BUILD)/$(src:.c=.o),$(src))))
define s_template
OBJS += $(1)
$(1): $(2)
	@mkdir -p $(dir $(1))
	$$(AS) $$(ASFLAGS) $$< -c -o $$@
endef
$(foreach src,$(S_SRCS),$(eval $(call s_template,$(BUILD)/$(src:.s=.o),$(src))))

$(OUT).elf: $(OBJS) $(LDSCRIPT)
	$(LD) $(LDFLAGS) $(OBJS) -o $@

%.lst: %.elf
	$(OD) -S $< > $@

%.bin: %.elf
	$(BIN) $< $@

%.hex: %.elf
	$(HEX) $< $@


-include $(shell [ -d $(BUILD) ] && find $(BUILD) -name \*.d)
