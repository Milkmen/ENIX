#include "enix.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "filesystem/filesystem.h"
#include "shell/shell.h"

void enix_loop()
{
    static char input[INPUT_BUFFER_SIZE];
    static int input_pos = 0;
    
    // Read one character at a time
    int c = getchar();
    
    if (c != EOF) 
    {
        if (c == '\r' || c == '\n') 
        {
            // End of line - process command
            printf("\n");
            input[input_pos] = '\0';
            
            if (input_pos > 0) 
            {
                sh_execute(input);
            }
            
            // Reset for next command
            input_pos = 0;
            printf("ENIX > ");
            fflush(stdout);
        }
        else if (c == '\b' || c == 127) 
        {
            // Backspace handling
            if (input_pos > 0) 
            {
                input_pos--;
                printf("\b \b");  // Move back, print space, move back again
                fflush(stdout);
            }
        }
        else if (c >= 32 && c < 127) 
        {
            // Printable character
            if (input_pos < INPUT_BUFFER_SIZE - 1) 
            {
                input[input_pos++] = c;
                putchar(c);  // Echo the character
                fflush(stdout);
            }
        }
    }
    
    vTaskDelay(pdMS_TO_TICKS(10));
}

void enix_entry()
{
    size_t rootfs_size;
    size_t mountfs_size;

    if(!fs_initialize(&rootfs_size, &mountfs_size))
    {
        ESP_LOGE("ENIX", "Failed to initialize filesystem");
        return;
    }

    if(!sh_initialize())
    {
        return;
    }


    char version_string[64] = { 0 };
    FILE* f = fopen("/rootfs/version.txt", "r");
    if (f) 
    {
        fgets(version_string, sizeof(version_string), f);
        fclose(f);
        
        // Remove newline if present
        version_string[strcspn(version_string, "\n")] = 0;
    }

    if(*version_string)
    {
        printf("ENIX Environment - v%s\n", version_string);
    }
    else 
    {
        printf("ENIX Environment - INVALID\n");
    }	
    
    printf("Filesystem initialized: %zu bytes total, %zu bytes used\n", rootfs_size, mountfs_size);
    printf("Type 'help' for available commands\n");
    printf("ENIX > ");
    fflush(stdout);
        
    while(1)
    {
        enix_loop();
    }
    
    fs_shutdown();
}