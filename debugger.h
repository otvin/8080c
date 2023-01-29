#ifndef DEBUGGER_8080_H
#define DEBUGGER_8080_H

#include <stdint.h>
#include <stdbool.h>
#include "cpu8080.h"
#include "motherboard.h"

void debug_dump_8080(cpu8080 cpu);
bool debug_8080(motherboard8080 motherboard, uint64_t *total_states, uint64_t *total_instructions);

#endif