#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>
#include "memory.h"

void handle_error(){
    perror("Fatal error");
    exit(EXIT_FAILURE);
}

uint8_t *init_memory(int memsize) {
    uint8_t *ptr;
    ptr = (uint8_t *) malloc (memsize * sizeof(uint8_t));
    if(!ptr){
        handle_error();
    }
    return ptr;
}

void destroy_memory(uint8_t **memory_ptr) {
    free(*memory_ptr);
}

void load_rom(char *rom_name, int start_at, uint8_t *memory){
    FILE *infile;
    int i;
    int byte;
    bool done;

    infile = fopen(rom_name, "r");
    if (infile == NULL) {
        handle_error();
    }
    i = start_at;
    done = false;
    while (!done) {
        byte = fgetc(infile);
        if (byte == EOF) {
            done = true;
        }
        else {
            memory[i] = (uint8_t) byte;
            i++;
        }
    }

    fclose(infile);
}

void load_cpm_shim(uint8_t *memory) {
    /*
    The tests are designed to be run in a system emulating the CP/M operating system.  Per 
    https://retrocomputing.stackexchange.com/questions/9361/test-emulated-8080-cpu-without-an-os, we can use a quick
    shim that puts a HLT at 0x0000 in memory, and a simple "print" function at 0x0005.  I copied the work from
    https://github.com/gergoerdi/clash-intel8080/blob/master/test/Hardware/Intel8080/TestBench.hs to put the following
    source in memory starting 0x0000.  The test ROMs load at 0x0100.
    */

    int i;

    uint8_t CPM_SHIM[29] = { 
        0x3e, 0x0a,        // 0x0000: exit:    MVI A, 0x0a
        0xd3, 0x00,        // 0x0002:          OUT 0
        0x76,              // 0x0004:          HLT

        0x3e, 0x02,        // 0x0005: message: MVI A, 0x02
        0xb9,              // 0x0007:          CMP C
        0xc2, 0x0f, 0x00,  // 0x0008:          JNZ 0x000f
        0x7b,              // 0x000B: putChr:  MOV A, E
        0xd3, 0x00,        // 0x000C:          OUT 0
        0xc9,              // 0x000E:          RET

        0x0e, 0x24,        // 0x000F: putStr:  MVI C, '$'
        0x1a,              // 0x0011: loop:    LDAX DE
        0xb9,              // 0x0012:          CMP C
        0xc2, 0x17, 0x00,  // 0x0013:          JNZ next
        0xc9,              // 0x0016:          RET
        0xd3, 0x00,        // 0x0017: next:    OUT 0
        0x13,              // 0x0019:          INX DE
        0xc3, 0x11, 0x00   // 0x001a:          JMP loop
    };

    for(i=0; i<29; i++) {
        memory[i] = CPM_SHIM[i];
    }
}