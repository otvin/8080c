#ifndef MEMORY_8080_H
#define MEMORY_8080_H

#include <stdint.h>

uint8_t *init_memory(int memsize);
void destroy_memory(uint8_t **memory_ptr);
void load_rom(char *rom_name, int start_at, uint8_t *memory);
void load_cpm_shim(uint8_t *memory);

#endif