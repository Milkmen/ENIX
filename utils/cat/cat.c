#include "../../main/enix/binloader/syscalls.h"

void _start(syscall_table_t* sys)
{
    const char* msg = "Hello from test binary!\n";
    sys->sys_write(1, msg, strlen(msg));
    sys->sys_exit(0);
}