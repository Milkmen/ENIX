#include "binloader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_heap_caps.h"

// Magic number for a.out (example, adjust as needed)
#define AOUT_MAGIC 0x0107 // just an example

// Assume some stack size for the loaded program
#define USER_STACK_SIZE 4096

// Simple prototype for jumping to entry (no args)
typedef void (*entry_point_t)(void);

void bin_launch(const char* binary_path)
{
    FILE* bin_file = fopen(binary_path, "rb");
    if (!bin_file) 
    {
        printf("Failed to open binary file: %s\n", binary_path);
        return;
    }

    // Read header
    bin_aout_header_t hdr;
    if (fread(&hdr, sizeof(bin_aout_header_t), 1, bin_file) != 1) 
    {
        printf("Failed to read a.out header\n");
        fclose(bin_file);
        return;
    }

    if (hdr.a_magic != AOUT_MAGIC) 
    {
        printf("Invalid a.out magic: 0x%08x\n", hdr.a_magic);
        fclose(bin_file);
        return;
    }

    size_t total_size = hdr.a_text + hdr.a_data + hdr.a_bss + USER_STACK_SIZE;

    size_t free_internal = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    printf("Free internal heap: %d bytes\n", (int)free_internal);
    if (free_internal < total_size) 
    {
        printf("Not enough memory to load binary (need %d bytes)\n", (int)total_size);
        fclose(bin_file);
        return;
    }

    // Allocate memory: contiguous block for text+data+bss+stack
    uint8_t* mem = malloc(total_size);
    if (!mem) 
    {
        printf("Malloc failed\n");
        fclose(bin_file);
        return;
    }

    // Load .text segment
    if (fread(mem, 1, hdr.a_text, bin_file) != hdr.a_text) 
    {
        printf("Failed to read .text segment\n");
        free(mem);
        fclose(bin_file);
        return;
    }

    // Load .data segment
    if (fread(mem + hdr.a_text, 1, hdr.a_data, bin_file) != hdr.a_data) 
    {
        printf("Failed to read .data segment\n");
        free(mem);
        fclose(bin_file);
        return;
    }

    // Zero initialize .bss segment
    memset(mem + hdr.a_text + hdr.a_data, 0, hdr.a_bss);

    fclose(bin_file);

    // Setup stack pointer at the end of allocated block
    uint8_t* stack_top = mem + total_size;

    // Calculate entry point relative to mem base
    // Assuming a_entry is an offset from start of .text
    entry_point_t entry = (entry_point_t)(mem + hdr.a_entry);

    printf("Launching binary at entry %p with stack top %p\n", entry, stack_top);

    // Setup stack and jump to entry point
    // NOTE: This is very platform-specific.
    // On ESP32, you likely need inline asm to set SP and jump,
    // here’s a conceptual example (does NOT compile as-is):

    asm volatile (
        "mov sp, %0\n"  // Set stack pointer
        "jmp %1\n"      // Jump to entry point
        :
        : "r"(stack_top), "r"(entry)
        : "sp"
    );

    // When the binary returns, free the memory
    free(mem);
}
