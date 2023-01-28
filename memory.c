#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
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

void load_rom(char *rom_name, int start_at, int finish_at, uint8_t *memory){
    FILE *infile;
    int i;
    uint8_t byte;

    infile = fopen(rom_name, "r");
    if (infile == NULL) {
        handle_error();
    }

    for(i=start_at; i<=finish_at; i++) {
        byte = fgetc(infile);
        memory[i] = byte;
    }

    fclose(infile);
}

