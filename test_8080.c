#include <stdint.h>
#include <stdlib.h>
#include "memory.h"
#include "disassembler.h"
#include "motherboard.h"
#include "debugger.h"



int main() {

    motherboard8080 motherboard;
    init_test_motherboard(&motherboard);

    disassemble(0x0, 0x0BF4, -1, motherboard.memory);
    disassemble(0x1000, 0x13FD, -1, motherboard.memory);
    disassemble(0x1400, 0x19BB, -1, motherboard.memory);
    disassemble(0x19D1, 0x1A10, -1, motherboard.memory);
    disassemble(0x1A32, 0x1A90, -1, motherboard.memory);

    debug_dump_8080(motherboard.cpu);

    destroy_motherboard(&motherboard);
    return EXIT_SUCCESS;
}