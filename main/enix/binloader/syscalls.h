#ifndef ENIX_SYSCALLS_H
#define ENIX_SYSCALLS_H

#include <stddef.h>

typedef struct 
{
    void (*sys_exit)(int status);
    int  (*sys_write)(int fd, const void* buf, size_t count);
}
syscall_table_t;

extern syscall_table_t g_syscalls;

#endif
