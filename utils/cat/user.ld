ENTRY(_start) 
SECTIONS 
{ 
    . = 0x40000000; 
    .text : { *(.literal) *(.text) *(.text.*) } 
    .data : { *(.data) *(.data.*) } 
    .bss : { *(.bss) *(.bss.*) *(COMMON) } 
} 
