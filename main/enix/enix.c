#include "enix.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "filesystem/filesystem.h"
#include "shell/shell.h"

void enix_loop()
{
    // Simple shell loop - you can enhance this
    char input[256];
    printf("enix> ");
    fflush(stdout);
    
    if (fgets(input, sizeof(input), stdin)) {
        // Remove newline
        input[strcspn(input, "\n")] = 0;
        shell_exec(input);
    }
    
    vTaskDelay(pdMS_TO_TICKS(10));  // Small delay to prevent watchdog issues
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

	char version_string[64] = { 0 };
	FILE* f = fopen("/spiffs/version.txt", "r");
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
		
    while(1)
    {
        enix_loop();
    }
    
    fs_shutdown();
}