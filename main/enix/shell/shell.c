#include "shell.h"
#include <stdio.h>
#include <string.h>

#include "../binloader/binloader.h"

char cmd_history[CMD_HISTORY_SIZE][INPUT_BUFFER_SIZE];

shell_state_t shell;

bool sh_initialize()
{
    memset(&shell, 0, sizeof(shell));
    strcpy(shell.path, "/rootfs");  // Set default path
    return true;
}

int sh_execute(const char* line)
{
    if (!line || strlen(line) == 0) 
    {
        return 0;  // Empty command
    }
    
    if (strcmp(line, "help") == 0) 
    {
        printf("Available commands:\n");
        printf("  help - Show this help\n");
        printf("  version - Show version\n");
        return 0;
    }
    else
    if (strcmp(line, "version") == 0) 
    {
        printf("ENIX Shell v1.0\n");
        return 0;
    }
    else
    if (strncmp(line, "run ", 4) == 0) 
    {
        bin_launch(line + 4);  // Skip "run " prefix
        return 0;
    }

    printf("Unknown command: %s\n", line);
    return -1;  // Command not found
}