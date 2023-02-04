#ifndef MOTHERBOARD_8080_H
#define MOTHERBOARD_8080_H

#include <stdint.h>

typedef struct motherboard8080 {
    uint8_t *memory;
    bool (*input_handler)(struct motherboard8080 *motherboard, uint8_t port, uint8_t *in);
    bool (*output_handler)(struct motherboard8080 *motherboard, uint8_t port, uint8_t out);
} motherboard8080;

void init_test_motherboard(motherboard8080 *motherboard);
void destroy_motherboard(motherboard8080 *motherboard);

#endif