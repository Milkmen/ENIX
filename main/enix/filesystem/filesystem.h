#ifndef ENIX_FILESYSTEM_H
#define ENIX_FILESYSTEM_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/*
    @brief Initializes the File-System
*/
bool fs_initialize(size_t* rootfs_size, size_t* mountfs_size);

/*
    @brief Shuts down the File-System
*/
void fs_shutdown();

#endif