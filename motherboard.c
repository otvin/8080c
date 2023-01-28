#include "motherboard.h"
#include "memory.h"


void load_space_invaders_roms(uint8_t *memory) {
    load_rom("invaders.h", 0x0000, 0x07FF, memory);
    load_rom("invaders.g", 0x0800, 0x0FFF, memory);
    load_rom("invaders.f", 0x1000, 0x17FF, memory);
    load_rom("invaders.e", 0x1800, 0x1FFF, memory);
}

void init_test_motherboard(motherboard8080 *motherboard) {
    // motherboard for the 8080 test programs
    init_cpu8080(&(motherboard->cpu));
    motherboard->memory = init_memory(0x10000);
    load_space_invaders_roms(motherboard->memory);
}

void destroy_motherboard(motherboard8080 *motherboard) {
    destroy_memory(&(motherboard->memory));
}