#include "shell.h"
#include <stdio.h>
#include <string.h>

char cmd_history[CMD_HISTORY_SIZE][INPUT_BUFFER_SIZE];

shell_state_t shell = { 0 };

bool shell_initialize()
{
    
}

int sh_execute(const char* line)
{
    if (!line || strlen(line) == 0) 
    {
        return 0;  // Empty command
    }
    
    // Basic command parsing - you can expand this
    if (strcmp(line, "help") == 0) 
    {
        printf("Available commands:\n");
        printf("  help - Show this help\n");
        printf("  version - Show version\n");
        return 0;
    }
    
    if (strcmp(line, "version") == 0) 
    {
        printf("ENIX Shell v1.0\n");
        return 0;
    }
    
    printf("Unknown command: %s\n", line);
    return -1;  // Command not found
}