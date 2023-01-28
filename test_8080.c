#include <stdint.h>
#include "memory.h"
#include "disassembler.h"

void load_roms(uint8_t *memory) {
    int i;
    load_rom("invaders.h", 0x0000, 0x07FF, memory);
    load_rom("invaders.g", 0x0800, 0x0FFF, memory);
    load_rom("invaders.f", 0x1000, 0x17FF, memory);
    load_rom("invaders.e", 0x1800, 0x1FFF, memory);
}

void main() {
    uint8_t *memory;

    memory = init_memory(0x10000);

    load_roms(memory);
    disassemble(0x0, 0x0BF4, -1, memory);
    disassemble(0x1000, 0x13FD, -1, memory);
    disassemble(0x1400, 0x19BB, -1, memory);
    disassemble(0x19D1, 0x1A10, -1, memory);
    disassemble(0x1A32, 0x1A90, -1, memory);

    free_memory(&memory);
}