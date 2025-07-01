#include "enix.h"

#include <stdlib.h>
#include <stdio.h>

#include "esp_log.h"
#include "filesystem/filesystem.h"
#include "shell/shell.h"

void enix_loop()
{

}

void enix_entry()
{
    size_t rootfs_size;
    size_t mountfs_size;

	if(!fs_initialize(&rootfs_size, &mountfs_size))
        return;

	char version_string[64] = { 0 };
	FILE* f = fopen("/spiffs/version.txt", "r");
	if (f) 
	{
	    fgets(version_string, sizeof(version_string), f);
	    fclose(f);
	}

	if(*version_string)
    {
		printf("ENIX Environment - v%s\n", version_string);
	}
	else 
	{
		printf("ENIX Environment - INVALID\n");
	}	
		
		
    while(1)
    {
        enix_loop();
    }
    
    fs_shutdown();
}