#ifndef DISASSEMBLER_8080_H
#define DISASSEMBLER_8080_H

#include <stdint.h>

void disassemble(int start_addr, int max_addr, int point_addr, uint8_t *memory);


#endif