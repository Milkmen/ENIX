#include "syscalls.h"
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void _sys_exit(int status)
{
    printf("[sys_exit] Binary exited with status %d\n", status);
    vTaskDelete(NULL);
}

int _sys_write(int fd, const void* buf, size_t count)
{
    if (fd == 1)
    {
        fwrite(buf, 1, count, stdout);
        fflush(stdout);
        return count;
    }
    return -1;
}

syscall_table_t g_syscalls =
{
    _sys_exit,
    _sys_write
};
