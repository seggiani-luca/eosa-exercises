# sources
SRC          := src
SCRIPTS      := $(SRC)/scripts
C_SOURCES    := $(shell find $(SRC) -name "*.c")
S_SOURCES    := $(shell find $(SRC) -name "*.s") $(shell find $(SRC) -name "*.S")
PROC_SOURCES := $(SRC)/user/process

# scripts
KERN_LSCRIPT := $(SCRIPTS)/kern_lscript.ld
PROC_LSCRIPT := $(SCRIPTS)/proc_lscript.ld
PROC_SCRIPT  := $(SCRIPTS)/make_procs.py

# objects
PROC_HEADER := $(PROC_SOURCES)/process.h
OUT         := out
C_OBJECTS   := $(C_SOURCES:%.c=$(OUT)/%.o)
S_OBJECTS   := $(S_SOURCES:%.s=$(OUT)/%.o)
BIN         := $(OUT)/kernel.axf

# tools
CC     := arm-none-eabi-gcc
CFLAGS := -mthumb -mcpu=cortex-m3 -nostartfiles -ffreestanding -fno-builtin -std=gnu17 -g

LFLAGS := -T $(PROC_LSCRIPT)
LFLAGS += -Xlinker -Map=$(OUT)/output.map -nostdlib -lgcc

PY     := python

QEMU   := qemu-system-arm
QFLAGS_UART := -machine mps2-an385 \
               -monitor null \
               -nographic \
               -serial stdio \
               -serial file:$(OUT)/uart1.log \
               -serial file:$(OUT)/uart2.log
QFLAGS_SHST := -machine mps2-an385 \
               -monitor null \
               -nographic \
               -chardev stdio,id=stdio0 \
               -semihosting-config enable=on,target=native,chardev=stdio0 \
               -serial none

QFLAGS := $(QFLAGS_UART)

GDB := gdb 

.PHONY: all clean run debug format attach processes

all: $(BIN)

$(BIN): $(PROC_LSCRIPT) $(C_OBJECTS) $(S_OBJECTS) 
	$(CC) $(CFLAGS) $(LFLAGS) $(S_OBJECTS) $(C_OBJECTS) -o $(BIN)

$(OUT)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT)/%.o: %.s
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT):
	mkdir -p $(OUT)

$(PROC_LSCRIPT): $(PROC_SOURCES) $(KERN_LSCRIPT) 
	$(PY) $(PROC_SCRIPT) -d $(PROC_SOURCES) -k $(KERN_LSCRIPT) -l $(PROC_LSCRIPT) -o $(PROC_HEADER)

run: $(BIN)
	$(QEMU) $(QFLAGS) -kernel $(BIN)

debug: $(BIN)
	$(QEMU) $(QFLAGS) -kernel $(BIN) -S -gdb tcp::1234

attach:
	$(GDB) $(BIN) -ex 'target remote localhost:1234'

clean:
	rm -rf $(OUT)
	rm -f $(PROC_LSCRIPT)
	rm -f $(PROC_HEADER)

format:
	find . -type f \( \
		-name '*.c' -o \
		-name '*.h' \
	\) -exec clang-format -i {} +
