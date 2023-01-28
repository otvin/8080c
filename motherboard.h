#ifndef MOTHERBOARD_8080_H
#define MOTHERBOARD_8080_H

#include "cpu8080.h"


typedef struct {
    cpu8080 cpu;
    uint8_t *memory;
} motherboard8080;

void init_test_motherboard(motherboard8080 *motherboard);
void destroy_motherboard(motherboard8080 *motherboard);

#endif