#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
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

int main(int argc, char *argv[]) {

    uint64_t total_states, num_states, total_instructions;
    double sec;
    bool run, debug_mode = false;
    clock_t start_time, end_time, diff;

    if (argc > 1) {
        if (strncmp(argv[1], "-debug", 6) == 0) {
            debug_mode = true;
        }
    }


    total_states = 0;
    total_instructions = 0;

    motherboard8080 motherboard;
    init_test_motherboard(&motherboard);
    
    load_cpm_shim(motherboard.memory);

    // all test ROMs are loaded starting 0x100.  
    load_rom("TST8080.COM", 0x100, motherboard.memory);

    start_time = clock();
    run = true;

    if (debug_mode) {
        run = debug_8080(motherboard, &total_states, &total_instructions);
    }

    while (run && (!motherboard.cpu.halted)) {
        run = cycle_cpu8080(&(motherboard.cpu), &num_states);
        if (run) {
            total_states = total_states + num_states;
            total_instructions++;
        }
        else {
            debug_8080(motherboard, &total_states, &total_instructions);
            run = false;
        }
    }
    end_time = clock();
    diff = end_time - start_time;
    sec =  ((double)diff) / ((double)CLOCKS_PER_SEC);

    printf("Duration in CPU time: %f sec\n", sec);
    printf("Num instructions: %ld\n", total_instructions);
    if (sec > 0) {
        printf("Performance: %f states per second\n", ((double)total_states) / sec);
    }

    destroy_motherboard(&motherboard);
    return EXIT_SUCCESS;
}