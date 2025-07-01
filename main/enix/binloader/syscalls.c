#include "syscalls.h"

void _sys_exit(int status)
{

}

int _sys_write(int fd, const void* buf, size_t count)
{

}

syscall_table_t g_syscalls =
{
    _sys_exit,
    _sys_write
};
