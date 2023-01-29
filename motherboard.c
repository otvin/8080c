#include <stdio.h>
#include "motherboard.h"
#include "memory.h"


void load_space_invaders_roms(uint8_t *memory) {
    load_rom("invaders.h", 0x0000, memory);
    load_rom("invaders.g", 0x0800, memory);
    load_rom("invaders.f", 0x1000, memory);
    load_rom("invaders.e", 0x1800, memory);
}

bool handle_test_output(uint8_t port, uint8_t out) {
    if (port == 0x0) {
        printf("%c", (char) out);
        return(true);
    }
    else {
        printf("Output port %02X not handled.", port);
        return(false);
    }
}

bool handle_test_input(uint8_t port, uint8_t *in) {
    // the 8080 CPU tests do not involve any input.
    printf("Input port %02X not handled.", port);
    return(false);
}

void init_test_motherboard(motherboard8080 *motherboard) {
    // motherboard for the 8080 test programs
    motherboard->memory = init_memory(0x10000);
    init_cpu8080(&(motherboard->cpu));
    motherboard->cpu.motherboard_memory = motherboard->memory;
    motherboard->cpu.motherboard_input_handler = &handle_test_input;
    motherboard->cpu.motherboard_output_handler = &handle_test_output;
    motherboard->cpu.pc = 0x100; // all the test ROMs begin execution at 0x100.
    
}

void destroy_motherboard(motherboard8080 *motherboard) {
    destroy_memory(&(motherboard->memory));
}