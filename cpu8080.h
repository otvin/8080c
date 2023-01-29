#ifndef CPU8080_H
#define CPU8080_H

#include <stdint.h>
#include <stdbool.h>



typedef struct {
    uint16_t pc;  // program counter
    uint16_t sp;  // stack pointer
    
    /* Used for the debugger to print the stack.  Whenever there is an explicit LD to SP, this value gets updated.  It does not get
       updated on increment/decrement to SP. */
    uint16_t stack_pointer_start; 

    uint8_t a;  // accumulator
    uint8_t b;  // b, c, d, e, h, l are registers
    uint8_t c; 
    uint8_t d;
    uint8_t e;
    uint8_t h; 
    uint8_t l;

    /* Interrupts can be disabled via the DI opcode, but the interrupt bit is also cleared whenever an interrupt is triggered.  So
       interupt handlers have to re-enable interrupts via the EI opcode.  EI enables interrupts after the instruction following the 
       EI is executed, so we need to track that state across cycles. */
    bool enable_interrupts_after_next_instruction;
    bool interrupts_enabled;

    // If the CPU is halted, it will only handle interrupts
    bool halted;

    // Flags are single-bit flip-flops in the 8080, not a register.
    bool zero_flag;  // false for not zero, true for zero.
    bool carry_flag; // false for no carry, true for carry.
    bool sign_flag;  // false for plus/positive, true for minus/negative
    bool parity_flag;// Note in i8080 the parity is based on number of bits set.  Odd number of bits = false, even number of bits = true
    bool auxiliary_carry_flag;

    // I want to separate the motherboard from the cpu, but this means that the ways the CPU communicates with the motherboard
    // have to be referenced somehow.  I could pass the memory and the input/output handlers around but that feels more clunky than
    // this.
    bool (*motherboard_input_handler)(uint8_t port, uint8_t *in);
    bool (*motherboard_output_handler)(uint8_t port, uint8_t out);
    // It would be very very bad for someone to call free() on the motherboard_memory because that would break the motherboard.
    uint8_t *motherboard_memory;
} cpu8080;

void init_cpu8080(cpu8080 *cpu);
bool cycle_cpu8080(cpu8080 *cpu, int *num_states);

#endif