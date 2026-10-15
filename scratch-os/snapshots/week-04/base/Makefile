# Embedded Operating Systems and Architecture course
# Licensed under the MIT License. See LICENSE in the repository root.
#
# Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>

# ARM bare-metal cross-compiler: runs on the host, emits Cortex-M code.
CC := arm-none-eabi-gcc
# Build output folder; a DEMO build gets its own (build/demo-rogue/) so
# the two never mix. `make clean` deletes build/.
BUILD_DIR := build$(if $(DEMO),/demo-$(DEMO))
# Final linked image.
BIN := $(BUILD_DIR)/scratch-os.axf

# The folders that hold our source files, one folder per OS layer.
SRC_DIRS := boot kernel drivers lib src
# Every .c and .s file in those folders, found automatically.
SOURCES := $(foreach d,$(SRC_DIRS),$(wildcard $(d)/*.c) $(wildcard $(d)/*.s))
# Map each source to its object file, e.g. kernel/irq.s -> build/kernel/irq.o.
OBJECTS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(patsubst %.s,$(BUILD_DIR)/%.o,$(SOURCES)))

# Cortex-M3 Thumb code, no C startup code (boot/startup.s is ours),
# freestanding (no C library), C17 with GNU extensions.
CFLAGS := -mthumb -mcpu=cortex-m3 -nostartfiles -ffreestanding -std=gnu17
# Headers: include/ and the project root, so "cmsdk.h" and "drivers/uart.h" both resolve.
CFLAGS += -Iinclude
CFLAGS += -I.
# No built-in printf etc.: our own lib/printf.c is a plain function.
CFLAGS += -fno-builtin
CFLAGS += -Wall -Wextra
# -O0 and full debug info: the code stays close to the source for stepping.
CFLAGS += -O0 -g3
# Not -Werror: a new compiler warning should not stop the build.

# Optional demo switch: `make DEMO=rogue` (or badptr, overflow) builds
# task B with one deliberate bug. Empty means no demo.
DEMO ?=
# Turn the word into a define, e.g. rogue -> -DDEMO_ROGUE.
ifeq ($(DEMO),rogue)
CFLAGS += -DDEMO_ROGUE
else ifeq ($(DEMO),badptr)
CFLAGS += -DDEMO_BADPTR
else ifeq ($(DEMO),overflow)
CFLAGS += -DDEMO_OVERFLOW
else ifneq ($(DEMO),)
# Any other word is a typo: stop with a clear message.
$(error Unknown DEMO "$(DEMO)": use rogue, badptr or overflow)
endif

# Our linker script, plus a map file showing where each symbol landed.
LDFLAGS := -T scripts/mps2_m3.ld
LDFLAGS += -Xlinker -Map=$(BUILD_DIR)/output.map
# No C library; libgcc only for compiler helpers (e.g. software division).
LDFLAGS += -nostdlib -lgcc

# QEMU command: MPS2-AN385 board, UART0 on this terminal, our image loaded.
QEMU := qemu-system-arm -machine mps2-an385 -monitor null -nographic -serial stdio -kernel $(BIN)

# These targets are command names, not files on disk.
.PHONY: all clean run debug run-trace

# Default goal: build the image.
all: $(BIN)

# Compile one .c file to a .o file, creating its build folder first.
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Assemble one .s file to a .o file (GCC passes it to the assembler).
$(BUILD_DIR)/%.o: %.s
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Link all objects into the final image.
$(BIN): $(OBJECTS) scripts/mps2_m3.ld
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJECTS) -o $(BIN)

# Build, then run in QEMU. Quit with Ctrl-A then X.
run: $(BIN)
	$(QEMU)

# Like `run`, but paused for GDB on TCP port 1234. Connect from a second
# terminal with: arm-none-eabi-gdb build/scratch-os.axf, then `target remote :1234`.
debug: $(BIN)
	$(QEMU) -S -gdb tcp::1234

# Like `run`, plus QEMU messages about unimplemented registers or devices.
run-trace: $(BIN)
	$(QEMU) -d unimp

# Remove every generated file, demo builds included.
clean:
	rm -rf build
