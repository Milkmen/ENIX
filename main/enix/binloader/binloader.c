#include "binloader.h"
#include "syscalls.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Magic number for a.out (adjust if needed)
#define AOUT_MAGIC 0x0107

// Assume some stack size for the loaded program
#define USER_STACK_SIZE 4096

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
    ctx->entry(ctx->syscalls);

    printf("[binloader] Binary returned, cleaning up.\n");
    free(ctx->alloc_base);
    free(ctx);

    vTaskDelete(NULL);
}

void bin_launch(const char* binary_path)
{
    FILE* bin_file = fopen(binary_path, "rb");
    if (!bin_file) 
    {
        printf("Failed to open binary file: %s\n", binary_path);
        return;
    }

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

    size_t total_size = hdr.a_text + hdr.a_data + hdr.a_bss;

    size_t free_internal = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    printf("[binloader] Free internal heap: %d bytes\n", (int)free_internal);
    if (free_internal < total_size) 
    {
        printf("Not enough memory to load binary (need %d bytes)\n", (int)total_size);
        fclose(bin_file);
        return;
    }

    uint8_t* mem = malloc(total_size);
    if (!mem) 
    {
        printf("Malloc failed\n");
        fclose(bin_file);
        return;
    }

    if (fread(mem, 1, hdr.a_text, bin_file) != hdr.a_text) 
    {
        printf("Failed to read .text segment\n");
        free(mem);
        fclose(bin_file);
        return;
    }

    if (fread(mem + hdr.a_text, 1, hdr.a_data, bin_file) != hdr.a_data) 
    {
        printf("Failed to read .data segment\n");
        free(mem);
        fclose(bin_file);
        return;
    }

    memset(mem + hdr.a_text + hdr.a_data, 0, hdr.a_bss);

    fclose(bin_file);

    entry_point_t entry = (entry_point_t)(mem + hdr.a_entry);

    bin_task_context_t* ctx = malloc(sizeof(bin_task_context_t));
    if (!ctx)
    {
        printf("Failed to allocate bin_task_context\n");
        free(mem);
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
        free(mem);
        free(ctx);
    }
}
