# Makefile for compiling user programs to ELF format for ENIX
# Assumes ESP-IDF toolchain is in PATH

CC = xtensa-esp32-elf-gcc
OBJDUMP = xtensa-esp32-elf-objdump
SIZE = xtensa-esp32-elf-size

# Compiler flags for standalone user programs
CFLAGS = -Os -ffunction-sections -fdata-sections -nostdlib -nostartfiles
CFLAGS += -mlongcalls -mtext-section-literals

# Linker flags
LDFLAGS = -Wl,--gc-sections -nostdlib -nostartfiles
LDFLAGS += -Wl,-T,user.ld -Wl,-e,_start

TARGET = cat
SOURCES = cat.c

all: $(TARGET).elf

$(TARGET).elf: $(SOURCES) user.ld
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SOURCES)
	$(SIZE) $@

# Create a simple linker script for user programs
user.ld:
	@echo "Creating linker script..."
	@cat > user.ld << 'EOF'
ENTRY(_start)
SECTIONS
{
    . = 0x40000000;
    
    .text : {
        *(.literal)
        *(.text)
        *(.text.*)
    }
    
    .data : {
        *(.data)
        *(.data.*)
    }
    
    .bss : {
        *(.bss)
        *(.bss.*)
        *(COMMON)
    }
}
EOF

# Debug info
debug: $(TARGET).elf
	$(OBJDUMP) -h $<
	$(OBJDUMP) -d $<

# Clean
clean:
	rm -f $(TARGET).elf user.ld

.PHONY: all debug clean