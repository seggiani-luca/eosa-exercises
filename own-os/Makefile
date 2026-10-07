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
CFLAGS := -mthumb -mcpu=cortex-m3 -nostartfiles -ffreestanding -std=gnu17
LFLAGS := -T $(LSCRIPT)
LFLAGS += -Xlinker -Map=$(OUT)/output.map -nostdlib -lgcc
QEMU   := qemu-system-arm
QFLAGS := -machine mps2-an385 \
          -monitor null \
          -nographic \
		  -serial stdio 

.PHONY: all clean run debug

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

clean:
	rm -rf $(OUT)
