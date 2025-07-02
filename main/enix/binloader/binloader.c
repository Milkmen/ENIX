#include "binloader.h"
#include "syscalls.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "esp_heap_caps.h"
#include "esp_cache.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Assume some stack size for the loaded program
#define USER_STACK_SIZE 8192  // Increased stack size

// Entry point signature: receives syscall table
typedef void (*entry_point_t)(syscall_table_t* syscalls);

typedef struct
{
    entry_point_t entry;
    syscall_table_t* syscalls;
    uint8_t* alloc_base;
    size_t alloc_size;
}
bin_task_context_t;

static void bin_task(void* arg)
{
    bin_task_context_t* ctx = (bin_task_context_t*)arg;

    printf("[binloader] Starting binary...\n");
    
    // Flush caches before executing
    esp_cache_msync(ctx->alloc_base, ctx->alloc_size);
    
    ctx->entry(ctx->syscalls);

    printf("[binloader] Binary returned, cleaning up.\n");
    heap_caps_free(ctx->alloc_base);
    free(ctx);

    vTaskDelete(NULL);
}

static bool validate_elf_header(const elf32_hdr_t* hdr)
{
    // Check ELF magic
    if (hdr->e_ident[EI_MAG0] != ELFMAG0 ||
        hdr->e_ident[EI_MAG1] != ELFMAG1 ||
        hdr->e_ident[EI_MAG2] != ELFMAG2 ||
        hdr->e_ident[EI_MAG3] != ELFMAG3) {
        printf("Invalid ELF magic\n");
        return false;
    }

    // Check if it's a 32-bit executable for Xtensa
    if (hdr->e_ident[4] != 1) { // EI_CLASS = ELFCLASS32
        printf("Not a 32-bit ELF\n");
        return false;
    }

    if (hdr->e_type != ET_EXEC) {
        printf("Not an executable ELF (type: %d)\n", hdr->e_type);
        return false;
    }

    if (hdr->e_machine != EM_XTENSA) {
        printf("Not an Xtensa ELF (machine: %d)\n", hdr->e_machine);
        return false;
    }

    return true;
}

static bool is_valid_user_address(uint32_t addr, size_t size)
{
    // ESP32 memory map - allow only safe RAM regions for user code
    // DRAM: 0x3F800000 - 0x3FC00000 (but be conservative)
    // IRAM: 0x40080000 - 0x400A0000 (but this is risky for user code)
    
    // For safety, only allow DRAM regions that are typically available
    uint32_t end_addr = addr + size;
    
    // Check for reasonable DRAM range (heap area)
    if (addr >= 0x3F800000 && end_addr <= 0x3FC00000) {
        return true;
    }
    
    // Reject ROM and other dangerous areas
    if (addr < 0x40000000 || (addr >= 0x40000000 && addr < 0x40080000)) {
        printf("[binloader] ERROR: Binary wants to load into ROM area (0x%08" PRIx32 ")\n", addr);
        return false;
    }
    
    // For now, be very restrictive - only allow heap-allocated memory
    return false;
}

void bin_launch(const char* binary_path)
{
    FILE* bin_file = fopen(binary_path, "rb");
    if (!bin_file) 
    {
        printf("Failed to open binary file: %s\n", binary_path);
        return;
    }

    // Read ELF header
    elf32_hdr_t elf_hdr;
    if (fread(&elf_hdr, sizeof(elf32_hdr_t), 1, bin_file) != 1) 
    {
        printf("Failed to read ELF header\n");
        fclose(bin_file);
        return;
    }

    if (!validate_elf_header(&elf_hdr)) 
    {
        fclose(bin_file);
        return;
    }

    printf("[binloader] Valid ELF file, entry point: 0x%08" PRIx32 "\n", elf_hdr.e_entry);

    // Read program headers
    if (elf_hdr.e_phnum == 0) 
    {
        printf("No program headers found\n");
        fclose(bin_file);
        return;
    }

    elf32_phdr_t* phdrs = malloc(elf_hdr.e_phnum * sizeof(elf32_phdr_t));
    if (!phdrs) 
    {
        printf("Failed to allocate memory for program headers\n");
        fclose(bin_file);
        return;
    }

    fseek(bin_file, elf_hdr.e_phoff, SEEK_SET);
    if (fread(phdrs, sizeof(elf32_phdr_t), elf_hdr.e_phnum, bin_file) != elf_hdr.e_phnum) 
    {
        printf("Failed to read program headers\n");
        free(phdrs);
        fclose(bin_file);
        return;
    }

    // Calculate total memory needed and validate addresses
    uint32_t min_addr = UINT32_MAX;
    uint32_t max_addr = 0;
    
    for (int i = 0; i < elf_hdr.e_phnum; i++) 
    {
        if (phdrs[i].p_type == PT_LOAD) 
        {
            // Validate that this segment can be safely loaded
            if (!is_valid_user_address(phdrs[i].p_vaddr, phdrs[i].p_memsz)) {
                printf("[binloader] ERROR: Segment %d has invalid address range\n", i);
                free(phdrs);
                fclose(bin_file);
                return;
            }
            
            if (phdrs[i].p_vaddr < min_addr) min_addr = phdrs[i].p_vaddr;
            if (phdrs[i].p_vaddr + phdrs[i].p_memsz > max_addr) 
                max_addr = phdrs[i].p_vaddr + phdrs[i].p_memsz;
        }
    }

    if (min_addr == UINT32_MAX) {
        printf("[binloader] No loadable segments found\n");
        free(phdrs);
        fclose(bin_file);
        return;
    }

    size_t total_size = max_addr - min_addr;
    printf("[binloader] Memory range: 0x%08" PRIx32 " - 0x%08" PRIx32 " (%d bytes)\n", 
           min_addr, max_addr, (int)total_size);

    // For ESP32, we need to load user binaries into a safe location
    // We'll use a different approach: load into heap and create a simple relocatable format
    
    // Check available memory  
    size_t free_internal = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    size_t free_exec = heap_caps_get_free_size(MALLOC_CAP_EXEC);
    
    printf("[binloader] Free internal heap: %d bytes\n", (int)free_internal);
    printf("[binloader] Free executable heap: %d bytes\n", (int)free_exec);
    
    if (free_exec < total_size) 
    {
        printf("[binloader] Not enough executable memory (need %d bytes)\n", (int)total_size);
        free(phdrs);
        fclose(bin_file);
        return;
    }

    // Allocate executable memory
    uint8_t* mem = heap_caps_malloc(total_size, MALLOC_CAP_EXEC | MALLOC_CAP_32BIT);
    if (!mem) 
    {
        printf("[binloader] Failed to allocate executable memory\n");
        free(phdrs);
        fclose(bin_file);
        return;
    }

    // Initialize memory to zero
    memset(mem, 0, total_size);

    // Load all PT_LOAD segments
    bool load_success = true;
    for (int i = 0; i < elf_hdr.e_phnum; i++) 
    {
        if (phdrs[i].p_type == PT_LOAD) 
        {
            uint32_t offset_in_mem = phdrs[i].p_vaddr - min_addr;
            
            printf("[binloader] Loading segment %d: vaddr=0x%08" PRIx32 ", filesz=%d, memsz=%d\n",
                   i, phdrs[i].p_vaddr, (int)phdrs[i].p_filesz, (int)phdrs[i].p_memsz);

            // Bounds check
            if (offset_in_mem + phdrs[i].p_memsz > total_size) {
                printf("[binloader] ERROR: Segment %d exceeds allocated memory\n", i);
                load_success = false;
                break;
            }

            // Read file data if present
            if (phdrs[i].p_filesz > 0) 
            {
                fseek(bin_file, phdrs[i].p_offset, SEEK_SET);
                if (fread(mem + offset_in_mem, 1, phdrs[i].p_filesz, bin_file) != phdrs[i].p_filesz) 
                {
                    printf("Failed to read segment %d\n", i);
                    load_success = false;
                    break;
                }
            }
        }
    }

    free(phdrs);
    fclose(bin_file);

    if (!load_success) {
        heap_caps_free(mem);
        return;
    }

    // Calculate entry point relative to our loaded memory
    uint32_t entry_offset = elf_hdr.e_entry - min_addr;
    if (entry_offset >= total_size) {
        printf("[binloader] ERROR: Entry point outside loaded memory\n");
        heap_caps_free(mem);
        return;
    }
    
    entry_point_t entry = (entry_point_t)(mem + entry_offset);
    
    printf("[binloader] Calculated entry point: %p (offset 0x%08" PRIx32 ")\n", 
           (void*)entry, entry_offset);

    bin_task_context_t* ctx = malloc(sizeof(bin_task_context_t));
    if (!ctx)
    {
        printf("Failed to allocate bin_task_context\n");
        heap_caps_free(mem);
        return;
    }
    ctx->entry = entry;
    ctx->syscalls = &g_syscalls;
    ctx->alloc_base = mem;
    ctx->alloc_size = total_size;

    printf("[binloader] Launching FreeRTOS task for binary.\n");

    BaseType_t res = xTaskCreate(
        bin_task,
        "user_bin",
        USER_STACK_SIZE / sizeof(StackType_t),
        ctx,
        tskIDLE_PRIORITY + 1,
        NULL
    );

    if (res != pdPASS)
    {
        printf("Failed to create task\n");
        heap_caps_free(mem);
        free(ctx);
    }
}