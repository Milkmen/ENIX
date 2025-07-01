#ifndef ENIX_BINLOADER_H
#define ENIX_BINLOADER_H

#include <stdint.h>

typedef struct 
{
    uint32_t a_magic;
    uint32_t a_text;
    uint32_t a_data;
    uint32_t a_bss;
    uint32_t a_syms;
    uint32_t a_entry;
    uint32_t a_trsize;
    uint32_t a_drsize;
} bin_aout_header_t;

/*
 @brief Launches a Binary Process
*/
void bin_launch(const char* binary_path);

#endif
