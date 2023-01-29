#include <stdio.h>
#include "cpu8080.h"

// There are some exceptions, where the number of states will change based on conditions.  Those will be handled in cycle().
// -1 is used for invalid opcodes.
int64_t states_per_opcode[256] = {
    4, 10, 7, 5, 5, 5, 7, 4, -1, 10, 7, 5, 5, 5, 7, 4,
    -1, 10, 7, 5, 5, 5, 7, 4, -1, 10, 7, 5, 5, 5, 7, 4,
    -1, 10, 16, 5, 5, 5, 7, 4, -1, 10, 16, 5, 5, 5, 7, 4,
    -1, 10, 13, 5, 10, 10, 4, -1, 10, 13, 5, 5, 5, 7, 4,
    5, 5, 5, 5, 5, 5, 7, 5, 5, 5, 5, 5, 5, 5, 7, 5,
    5, 5, 5, 5, 5, 5, 7, 5, 5, 5, 5, 5, 5, 5, 7, 5,
    5, 5, 5, 5, 5, 5, 7, 5, 5, 5, 5, 5, 5, 5, 7, 5,
    7, 7, 7, 7, 7, 7, 7, 7, 5, 5, 5, 5, 5, 5, 7, 5,
    4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
    4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
    4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
    4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
    5, 10, 10, 10, 11, 11, 7, 11, 5, 10, 10, -1, 11, 17, 7, 11,
    5, 10, 10, 10, 11, 11, 7, 11, 5, -1, 10, 10, 11, -1, 7, 11,
    5, 10, 10, 18, 11, 11, 7, 11, 5, 5, 10, 5, 11, -1, 7, 11,
    5, 10, 10, 4, 11, 11, 7, 11, 5, 5, 10, 4, 11, -1, 7, 11
};

void init_cpu8080(cpu8080 *cpu) {
    cpu->pc = 0x0;
    cpu->sp = 0x0;
    cpu->stack_pointer_start = 0x0;
    cpu->a = 0x0;
    cpu->b = 0x0;
    cpu->c = 0x0;
    cpu->d = 0x0;
    cpu->e = 0x0;
    cpu->h = 0x0;
    cpu->l = 0x0;
    cpu->enable_interrupts_after_next_instruction = false;
    cpu->interrupts_enabled = true;
    cpu->halted = false;
    cpu->zero_flag = false;
    cpu->carry_flag = false;
    cpu->sign_flag = false;
    cpu->auxiliary_carry_flag = false;
}

void set_zero_sign_parity_from_byte(cpu8080 *cpu, uint8_t byte) {
    uint8_t v;
    
    cpu->zero_flag = (bool) (byte == 0);

    // Per the Assembly Language Programming Manual, the sign flag is set to the value of bit 7.
    cpu->sign_flag = (bool) (byte & 0x8);

    // http://www.graphics.stanford.edu/~seander/bithacks.html#ParityParallel
    v = byte ^ (byte >> 4);
    v = v & 0xf;
    cpu->parity_flag = (bool) (((0x6996 >> v) & 1) == 0);
}

uint8_t get_byte_from_flags(cpu8080 cpu){
    /* Because there are only 5 condition flags, PUSH PSW formats the flags into an 8-bit byte by
       setting bits 3 and 5 always to zero and bit one is always set to 1. */
    uint8_t retval = 0x02;
    if (cpu.sign_flag) retval = retval | 0x80;
    if (cpu.zero_flag) retval = retval | 0x40;
    if (cpu.auxiliary_carry_flag) retval = retval | 0x10;
    if (cpu.parity_flag) retval = retval | 0x04;
    if (cpu.carry_flag) retval = retval | 0x01;

    return retval;
}

void set_flags_from_byte(cpu8080 *cpu, uint8_t byte) {
    cpu->sign_flag = (bool) (byte & 0x80);
    cpu->zero_flag = (bool) (byte & 0x40);
    cpu->auxiliary_carry_flag = (bool) (byte & 0x10);
    cpu->parity_flag = (bool) (byte & 0x04);
    cpu->carry_flag = (bool) (byte & 0x01);
}
