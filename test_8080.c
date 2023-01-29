#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdbool.h>
#include "memory.h"
#include "disassembler.h"
#include "motherboard.h"
#include "debugger.h"
#include "cpu8080.h"

/*
Virtual computer to run 8080 Emulator tests.  Tests may be found at https://altairclone.com/downloads/cpu_tests/
*/  

int main() {

    uint64_t total_states, total_instructions;
    int num_states;
    bool run;

    total_states = 0;
    total_instructions = 0;

    motherboard8080 motherboard;
    init_test_motherboard(&motherboard);
    
    load_cpm_shim(motherboard.memory);

    // all test ROMs are loaded starting 0x100.  
    load_rom("TST8080.COM", 0x100, motherboard.memory);
    disassemble(0x0, 0x20, -1, motherboard.memory);
    disassemble(0x100, 1792, -1, motherboard.memory);
    debug_dump_8080(motherboard.cpu);

    run = true;
    while (run && (!motherboard.cpu.halted)) {
        run = cycle_cpu8080(&(motherboard.cpu), &num_states);
        if (run) {
            total_states = total_states + num_states;
            total_instructions++;
        }
        else {
            printf("ERROR\n");
        }
    }



    destroy_motherboard(&motherboard);
    return EXIT_SUCCESS;
}