# sources
SRC       := src
SCRIPTS   := $(SRC)/scripts
LSCRIPT   := $(SCRIPTS)/lscript.ld
C_SOURCES := $(shell find $(SRC) -name "*.c")
S_SOURCES := $(shell find $(SRC) -name "*.s")

# objects
OUT       := out
C_OBJECTS := $(C_SOURCES:%.c=$(OUT)/%.o)
S_OBJECTS := $(S_SOURCES:%.s=$(OUT)/%.o)
BIN       := $(OUT)/kernel.axf

# tools
CC     := arm-none-eabi-gcc
CFLAGS := -mthumb -mcpu=cortex-m3 -nostartfiles -ffreestanding -std=gnu17 -g

LFLAGS := -T $(LSCRIPT)
LFLAGS += -Xlinker -Map=$(OUT)/output.map -nostdlib -lgcc

QEMU   := qemu-system-arm
QFLAGS_UART := -machine mps2-an385 \
               -monitor null \
               -nographic \
               -serial stdio
QFLAGS_SHST := -machine mps2-an385 \
               -monitor null \
               -nographic \
               -chardev stdio,id=stdio0 \
               -semihosting-config enable=on,target=native,chardev=stdio0 \
               -serial none

QFLAGS := $(QFLAGS_UART)

GDB := arm-none-eabi-gdb

.PHONY: all clean run debug format

all: $(BIN)

$(BIN): $(C_OBJECTS) $(S_OBJECTS) 
	$(CC) $(CFLAGS) $(LFLAGS) $(S_OBJECTS) $(C_OBJECTS) -o $(BIN)

$(OUT)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT)/%.o: %.s
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT):
	mkdir -p $(OUT)

run: $(BIN)
	$(QEMU) $(QFLAGS) -kernel $(BIN)

debug: $(BIN)
	$(QEMU) $(QFLAGS) -kernel $(BIN) -S -gdb tcp::1234

gdb:
	$(GDB) $(BIN) -ex 'target remote localhost:1234'

clean:
	rm -rf $(OUT)

format:
	find . -type f \( \
		-name '*.c' -o \
		-name '*.h' \
	\) -exec clang-format -i {} +
