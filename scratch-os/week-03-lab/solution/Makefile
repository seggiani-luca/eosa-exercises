# Embedded Operating Systems and Architecture course
# Licensed under the MIT License. See LICENSE in the repository root.
#
# Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
#
# WEEK 3 LAB SOLUTION - not a checkpoint. Nothing later in the course
# builds on this folder.
#
# Derived from ../../week-02-lab/solution/Makefile, the Makefile the
# student wrote in week-02-lab (A2) and extended (B1). starter/ ships no
# Makefile: the student brings that one. Two exercises change it:
#   lab A1: adapt it to a boot file written in assembly (SOURCES, the
#           OBJECTS derivation, and a pattern rule for .s sources);
#   lab A2: add the `debug` target and the QEMU_EXTRA hook.

# --- Variables (notes section 6, "Variables") ----------------------------

# The cross-compiler: runs on the host, emits Cortex-M code (section 2).
CC := arm-none-eabi-gcc
# Every generated file goes here, so `clean` is one rm -rf.
BUILD_DIR := build
# The final linked image. .axf is ARM's name for an ELF with debug info.
BIN := $(BUILD_DIR)/scratch-os.axf

# lab A1: the boot code is now boot/startup.s, not boot/startup.c.
SOURCES := boot/startup.s src/main.c
# lab A1: a substitution reference leaves any word it does not match
# unchanged. With only the first line below, boot/startup.s would pass
# through untouched, land in OBJECTS as a source path, and be assembled
# during the link - building and running correctly while no
# build/boot/startup.o is ever made. The second line maps the .s words.
OBJECTS := $(SOURCES:%.c=$(BUILD_DIR)/%.o)
OBJECTS := $(OBJECTS:%.s=$(BUILD_DIR)/%.o)

# Target policy (notes section 2).
# -ffreestanding: tells the compiler that a hosted environment's
#  guarantees - a full standard C library, a distinguished main - do not
#  hold here. Together with -nostdlib below it is what makes "no C
#  library" true rather than merely intended.
CFLAGS := -mthumb -mcpu=cortex-m3 -nostartfiles -ffreestanding -std=gnu17
CFLAGS += -Wall -Wextra
CFLAGS += -O0 -g3

# Our linker script, plus a map file (notes section 3).
LDFLAGS := -T scripts/mps2_m3.ld
LDFLAGS += -Xlinker -Map=$(BUILD_DIR)/output.map
# -nostdlib: do not link a hosted libc or its startup code; -lgcc still
# supplies libgcc's software helpers. Matches ../../week-02/solution.
LDFLAGS += -nostdlib -lgcc

# lab A2: the emulator invocation, split in two. QEMUFLAGS is what every
# run needs - without -semihosting nothing the program prints reaches
# this terminal - and is never meant to be replaced. QEMU_EXTRA is empty
# and exists to be set on the command line, so an extra option is ADDED:
#     make run QEMU_EXTRA="-d unimp,guest_errors"
# Setting QEMUFLAGS there instead would replace the whole list.
QEMUFLAGS := -machine mps2-an385 -monitor null -semihosting \
             --semihosting-config enable=on,target=native \
             -kernel $(BIN) -serial stdio -nographic
QEMU_EXTRA :=

# --- Phony targets (notes section 6, "Phony targets") --------------------

.PHONY: all clean run debug size disasm

all: $(BIN)

# --- Rules (notes section 6, "Automatic variables" / "Pattern rules") ----

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# lab A1: the same rule for hand-written assembly. gcc hands a .s file
# straight to the assembler; passing CFLAGS keeps -mthumb -mcpu in step
# with the C objects.
$(BUILD_DIR)/%.o: %.s
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN): $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJECTS) -o $(BIN)

# Boot the image under QEMU's MPS2-AN385 model (notes section 7).
run: $(BIN)
	qemu-system-arm $(QEMUFLAGS) $(QEMU_EXTRA)

# lab A2: the run command plus two options (lecture-notes/week-02.md,
# "A remote debugger: two programs, two terminals"): -S holds the CPU at
# reset, -gdb tcp::1234 waits for a debugger on that port.
debug: $(BIN)
	qemu-system-arm $(QEMUFLAGS) -S -gdb tcp::1234 $(QEMU_EXTRA)

clean:
	rm -rf $(BUILD_DIR)

# --- Carried over from week-02-lab B1 ------------------------------------

size: $(BIN)
	arm-none-eabi-size $(BIN)

disasm: $(BIN)
	arm-none-eabi-objdump -d $(BIN)
