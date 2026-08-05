TARGET = firmware

CC      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE    = arm-none-eabi-size

# Soft-float: no FPU bring-up required for this LED test
MCU     = -mcpu=cortex-m4 -mthumb

CFLAGS  = $(MCU) -O0 -Wall -g -DSTM32L475xx
LDFLAGS = $(MCU) -Tstm32l475vg.ld -nostartfiles \
          --specs=nano.specs --specs=nosys.specs \
          -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--gc-sections

BUILD_DIR   = build
C_SOURCES   = src/main.c
ASM_SOURCES = startup_stm32l475xx.s

OBJECTS  = $(addprefix $(BUILD_DIR)/, $(notdir $(C_SOURCES:.c=.o)))
OBJECTS += $(addprefix $(BUILD_DIR)/, $(notdir $(ASM_SOURCES:.s=.o)))

.PHONY: all clean flash

all: $(BUILD_DIR)/$(TARGET).elf

$(BUILD_DIR)/%.o: src/%.c | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: %.s | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(SIZE) $@

$(BUILD_DIR):
	mkdir -p $@

clean:
	rm -rf $(BUILD_DIR)

# Explicit reset run — "program ... reset" can leave the core halted
flash: $(BUILD_DIR)/$(TARGET).elf
	openocd -f interface/stlink.cfg -f target/stm32l4x.cfg \
	  -c "program $(BUILD_DIR)/$(TARGET).elf verify; reset run; shutdown"
