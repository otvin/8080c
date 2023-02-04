#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <stdbool.h>
#include "memory.h"
#include "disassembler.h"
#include "cpu8080.h"
#include "motherboard.h"
#include "debugger.h"


/*
Virtual computer to run 8080 Emulator tests.  Tests may be found at https://altairclone.com/downloads/cpu_tests/
*/  

int main(int argc, char *argv[]) {

    uint64_t total_states, num_states, total_instructions;
    double sec;
    bool run, debug_mode = false;
    clock_t start_time, end_time, diff;
    struct timeval start_time1, end_time1;
    double sec1;

    if (argc > 1) {
        if (strncmp(argv[1], "-debug", 6) == 0) {
            debug_mode = true;
        }
    }


    total_states = 0;
    total_instructions = 0;

    motherboard8080 motherboard;
    cpu8080 cpu;
    init_test_cpu8080(&cpu);
    init_test_motherboard(&motherboard);
    
    load_cpm_shim(motherboard.memory);

    // all test ROMs are loaded starting 0x100.  
    // load_rom("TST8080.COM", 0x100, motherboard.memory);
    // load_rom("8080PRE.COM", 0x100, motherboard.memory);
    // load_rom("CPUTEST.COM", 0x100, motherboard.memory);
    load_rom("8080EXM.COM", 0x100, motherboard.memory);
    
    start_time = clock();
    gettimeofday(&start_time1, NULL);
    run = true;

    if (debug_mode) {
        run = debug_8080(&motherboard, &cpu, &total_states, &total_instructions);
    }

    while (run && (!cpu.halted)) {
        run = cycle_cpu8080(&motherboard, &cpu, &num_states);
        if (run) {
            total_states = total_states + num_states;
            total_instructions++;
        }
        else {
            debug_8080(&motherboard, &cpu, &total_states, &total_instructions);
            run = false;
        }
    }
    end_time = clock();
    gettimeofday(&end_time1, NULL);
    diff = end_time - start_time;
    sec =  ((double)diff) / ((double)CLOCKS_PER_SEC);
    sec1 = ((double)(end_time1.tv_usec - start_time1.tv_usec) / 1000000) + ((double)(end_time1.tv_sec - start_time1.tv_sec));

    printf("Duration in CPU time: %f sec\n", sec);
    printf("Duration in clock time: %f sec\n", sec1);
    printf("Num instructions: %ld\n", total_instructions);
    if (sec > 0) {
        printf("Performance: %f states per CPU second\n", ((double)total_states) / sec);
    }
    if (sec1 > 0) {
        printf("Performance: %f states per clock second\n", ((double)total_states) / sec1);
    }

    destroy_motherboard(&motherboard);
    return EXIT_SUCCESS;
}