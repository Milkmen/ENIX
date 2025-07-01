#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

bool fs_initialize(size_t* rootfs_size, size_t* mountfs_size);
void fs_shutdown();

#endif