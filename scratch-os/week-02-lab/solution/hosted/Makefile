# Embedded Operating Systems and Architecture course
# Licensed under the MIT License. See LICENSE in the repository root.
#
# Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
#
# WEEK 2 LAB SOLUTION - not a checkpoint.
#
# This Makefile is PROVIDED, not an exercise. Exercise A2 asks you to
# construct the firmware build one directory up; this one builds a
# different kind of program and is scaffolding for exercise A3.
#
# The difference from the firmware build is entirely in the link:
#
#   --specs=rdimon.specs   links newlib's semihosting C library, so the
#                          standard library - printf included - is
#                          present and routed to the QEMU console.
#   (no -nostartfiles)     newlib supplies its own startup code, which
#                          is what performs the C run-time
#                          initialisation that printf depends on.
#   --defsym flags         newlib's startup code refers to .bss and heap
#                          bounds by its own symbol names. These three
#                          definitions map them onto the names
#                          mps2_m3.ld already defines, so that the SAME,
#                          unmodified linker script serves both builds.
#
# The compiler, the processor, the instruction set and the QEMU command
# are identical to the firmware build. Only the execution environment
# differs: this program is HOSTED, the firmware image is FREESTANDING
# (ISO C section 4).

CC := arm-none-eabi-gcc
BUILD_DIR := build
BIN := $(BUILD_DIR)/swap.axf

# Identical to the firmware build's flags, less -nostartfiles.
CFLAGS := -mthumb -mcpu=cortex-m3 -std=gnu17 -Wall -Wextra -O0 -g3

LDFLAGS := --specs=rdimon.specs
LDFLAGS += -T ../scripts/mps2_m3.ld
LDFLAGS += -Wl,--defsym,__bss_start__=_sbss
LDFLAGS += -Wl,--defsym,__bss_end__=_ebss
LDFLAGS += -Wl,--defsym,__end__=_heap_bottom

.PHONY: all run clean size

all: $(BIN)

$(BIN): swap.c ../boot/startup.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

run: $(BIN)
	qemu-system-arm -machine mps2-an385 -monitor null -semihosting \
	    --semihosting-config enable=on,target=native \
	    -kernel $(BIN) -serial stdio -nographic

size: $(BIN)
	arm-none-eabi-size $(BIN)

clean:
	rm -rf $(BUILD_DIR)
