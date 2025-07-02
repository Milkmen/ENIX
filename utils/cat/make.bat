@echo off
echo Creating linker script...
echo ENTRY(_start) > user.ld
echo SECTIONS >> user.ld
echo { >> user.ld
echo     . = 0x40000000; >> user.ld
echo     .text : { *(.literal) *(.text) *(.text.*) } >> user.ld
echo     .data : { *(.data) *(.data.*) } >> user.ld
echo     .bss : { *(.bss) *(.bss.*) *(COMMON) } >> user.ld
echo } >> user.ld

echo Compiling cat.c...
xtensa-esp32-elf-gcc -Os -ffunction-sections -fdata-sections -nostdlib -nostartfiles -mlongcalls -mtext-section-literals -Wl,--gc-sections -Wl,-T,user.ld -Wl,-e,_start -o cat.elf cat.c

echo Done!
xtensa-esp32-elf-size cat.elf