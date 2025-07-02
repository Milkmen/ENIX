#include "../../main/enix/binloader/syscalls.h"

// Simple strlen implementation since we're using -nostdlib
static int my_strlen(const char* str) {
    int len = 0;
    while (str[len]) len++;
    return len;
}

void _start(syscall_table_t* sys)
{
    const char* msg = "Hello from ELF binary!\n";
    sys->sys_write(1, msg, my_strlen(msg));
    sys->sys_exit(0);
}