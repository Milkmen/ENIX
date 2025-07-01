#ifndef ENIX_SHELL_H
#define ENIX_SHELL_H

#include <stdbool.h>
#include <stdint.h>

#define CMD_HISTORY_SIZE    2
#define INPUT_BUFFER_SIZE   80

typedef struct 
{
    char path[INPUT_BUFFER_SIZE];
}
shell_state_t;

/*
    @brief Initializes the Shell
*/
bool sh_initialize();

/*
    @brief Executes a Shell Command
*/
int sh_execute(const char* line);

#endif