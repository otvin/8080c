#ifndef MEMORY_8080_H
#define MEMORY_8080_H

#include <stdint.h>

uint8_t *init_memory(int memsize);
void free_memory(uint8_t **memory_ptr);
void load_rom(char *rom_name, int start_at, int finish_at, uint8_t *memory);


#endif