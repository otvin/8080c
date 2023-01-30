#include <stdio.h>
#include "cpu8080.h"

#define GET_BC (((cpu->b) << 8) | (cpu->c))
#define GET_DE (((cpu->d) << 8) | (cpu->e))
#define GET_HL (((cpu->h) << 8) | (cpu->l))

// There are some exceptions, where the number of states will change based on conditions.  Those will be handled in cycle().
// -1 is used for invalid opcodes.  Using 64-bit ints because they will get added to a 64-bit int and this
// avoids a cast later.
const int64_t states_per_opcode[256] = {
    4, 10, 7, 5, 5, 5, 7, 4, -1, 10, 7, 5, 5, 5, 7, 4,
    -1, 10, 7, 5, 5, 5, 7, 4, -1, 10, 7, 5, 5, 5, 7, 4,
    -1, 10, 16, 5, 5, 5, 7, 4, -1, 10, 16, 5, 5, 5, 7, 4,
    -1, 10, 13, 5, 10, 10, 10, 4, -1, 10, 13, 5, 5, 5, 7, 4,
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
    5, 10, 10, 18, 11, 11, 7, 11, 5, 5, 10, 4, 11, -1, 7, 11,
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

    cpu->motherboard_memory = NULL;
    cpu->motherboard_input_handler = NULL;
    cpu->motherboard_output_handler = NULL;
}

void set_zero_sign_parity_from_byte(cpu8080 *cpu, uint8_t byte) {
    uint8_t v;
    
    cpu->zero_flag = (bool) (byte == 0);

    // Per the Assembly Language Programming Manual, the sign flag is set to the value of bit 7.
    cpu->sign_flag = (bool) (byte & 0x80);

    // http://www.graphics.stanford.edu/~seander/bithacks.html#ParityParallel
    v = byte ^ (byte >> 4);
    v = v & 0xf;
    cpu->parity_flag = (bool) (((0x6996 >> v) & 1) == 0);
}

uint8_t get_byte_from_flags(cpu8080 *const cpu){
    /* Because there are only 5 condition flags, PUSH PSW formats the flags into an 8-bit byte by
       setting bits 3 and 5 always to zero and bit one is always set to 1. */
    uint8_t retval = 0x02;
    if (cpu->sign_flag) retval = retval | 0x80;
    if (cpu->zero_flag) retval = retval | 0x40;
    if (cpu->auxiliary_carry_flag) retval = retval | 0x10;
    if (cpu->parity_flag) retval = retval | 0x04;
    if (cpu->carry_flag) retval = retval | 0x01;

    return retval;
}

void set_flags_from_byte(cpu8080 *cpu, uint8_t byte) {
    cpu->sign_flag = (bool) (byte & 0x80);
    cpu->zero_flag = (bool) (byte & 0x40);
    cpu->auxiliary_carry_flag = (bool) (byte & 0x10);
    cpu->parity_flag = (bool) (byte & 0x04);
    cpu->carry_flag = (bool) (byte & 0x01);
}

static inline void do_conditional_call(cpu8080 *cpu, bool flag, uint64_t *num_states, uint16_t *pc_increments) {
    if (flag) {
        cpu->motherboard_memory[cpu->sp - 1] = ((cpu->pc + 3) >> 8);
        cpu->motherboard_memory[cpu->sp - 2] = ((cpu->pc + 3) & 0xFF);
        cpu->sp = cpu ->sp - 2;
        cpu->pc = (cpu->motherboard_memory[cpu->pc+2] << 8) | cpu->motherboard_memory[cpu->pc+1];
        (*pc_increments) = 0;
        (*num_states) = 17;
    }
    else {
        (*pc_increments) = 3;
    }
}

static inline void do_conditional_jump(cpu8080 *cpu, bool flag, uint16_t *pc_increments) {
    if (flag) {
        cpu->pc = (cpu->motherboard_memory[cpu->pc+2] << 8) | cpu->motherboard_memory[cpu->pc+1];
        (*pc_increments) = 0;
    }
    else {
        (*pc_increments) = 3;
    }
}

static inline void do_conditional_return(cpu8080 *cpu, bool flag, uint64_t *num_states, uint16_t *pc_increments){
    if (flag) {
        cpu->pc = (cpu->motherboard_memory[cpu->sp +1] << 8) | cpu->motherboard_memory[cpu->sp];
        cpu->sp = cpu->sp + 2;
        (*pc_increments) = 0;
        (*num_states) = 11;
    }
    // otherwise - NOP - move to next instruction
}

static inline void do_addition(cpu8080 *cpu, uint8_t byte, bool add_one_for_carry) {
    uint8_t a_low, byte_low, low_tmp;
    uint16_t tmp; // avoids overflow
    // There is likely a better-performing way to compute AC but this is easy to understand.  If adding the low nibble causes a carry,
    // then the AC flag is true.
    a_low = cpu->a & 0x0F;
    byte_low = byte & 0x0F;
    low_tmp = a_low + byte_low;
    if (add_one_for_carry) {
        low_tmp++;
    }
    cpu->auxiliary_carry_flag = (bool)(low_tmp > 0xF);

    tmp = (add_one_for_carry) ? byte + 1 : byte;
    tmp = tmp + cpu->a;
    cpu->carry_flag = (bool) (tmp > 0xFF);
    cpu->a = (uint8_t)(tmp & 0xFF);
    set_zero_sign_parity_from_byte(cpu, cpu->a);
}

static inline void do_subtraction(cpu8080 *cpu, uint8_t byte, bool subtract_one_for_borrow, bool store_value) {
    /* byte is the value subtracted from the accumulator.  If store_value is false, does a CMP and doesn't save the value to a. */
    uint16_t tmp, q;  // avoids overflow
    tmp = (subtract_one_for_borrow) ? byte + 1 : byte;
    cpu -> carry_flag = (bool)((uint16_t) (cpu->a) < tmp);
    q = ((uint16_t)(cpu->a)) - tmp;
    set_zero_sign_parity_from_byte(cpu, (uint8_t)(q & 0xFF));
    // taking logic from MAME emulator
    cpu->auxiliary_carry_flag = (bool)((~(cpu->a ^ ((uint8_t)(q & 0xFF)) ^ byte)) & 0x10);
    if (store_value) {
        cpu->a = (uint8_t)(q & 0xFF);
    }
}

static inline void do_and(cpu8080 *cpu, uint8_t byte) {
    /* Note for ANI, the Assembly Language Programming Manual says that AC flag is set same as ANA, but the Intel 8080 
       Microcomputer Systems Manual says that the AC flag is cleared.  The various CPU tests expect it to be set, so 
       I am setting the flags on both ANA and ANI. */
    /* Per https://retrocomputing.stackexchange.com/questions/14977/auxiliary-carry-and-the-intel-8080s-logical-instructions
       the Auxiliary Carry flag is set to the or of bit 3 (0x08) of the 2 values involved in the AND operation. */
    cpu->auxiliary_carry_flag = (bool)((cpu->a | byte) & 0x08);
    cpu->a = cpu->a & byte;
    cpu->carry_flag = false;
    set_zero_sign_parity_from_byte(cpu, cpu->a);
}

static inline void do_or(cpu8080 *cpu, uint8_t byte) {
    cpu->a = cpu->a | byte;
    cpu->auxiliary_carry_flag = false;
    cpu->carry_flag = false;
    set_zero_sign_parity_from_byte(cpu, cpu->a);
}

static inline void do_xor(cpu8080 *cpu, uint8_t byte) {
    cpu->a = cpu->a ^ byte;
    cpu->auxiliary_carry_flag = false;
    cpu->carry_flag = false;
    set_zero_sign_parity_from_byte(cpu, cpu->a);
}

bool do_opcode(cpu8080 *cpu, uint64_t *num_states) {
    uint8_t opcode;
    uint16_t pc_increments = 1;  // assume we increment pc by one at the end of the function; instructions may override this.
    uint8_t tmp_d, tmp_e; // used in XCHG
    uint16_t tmp_rp; // used in opcodes like INX, DCX where we operate on a register pair

    opcode = cpu->motherboard_memory[cpu->pc];
    *num_states = states_per_opcode[opcode];
    if ((*num_states) == -1) {
        printf("Invalid Opcode %02X\n", opcode);
        return(false);
    }

    switch(opcode) {
        case 0x00: 
            // NOP
            // do nothing
            break;
        case 0x01:
            // LXI BC, data 16
            cpu->b = cpu->motherboard_memory[cpu->pc + 2];
            cpu->c = cpu->motherboard_memory[cpu->pc + 1];
            pc_increments = 3;
            break;
        case 0x02: // STAX
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "STAX (BC)");
            break;
        case 0x03: // INX
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "INC BC\t; INX rp");
            break;
        case 0x04: // INR
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "INC B\t; INR r");
            break;
        case 0x05: // DCR
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "DEC B\t; DCR r");
            break;
        case 0x06: 
            // MVI B, d8  (LD B, d8)
            cpu->b = cpu->motherboard_memory[cpu->pc + 1];
            pc_increments = 2;
            break;
        case 0x07: // RLC
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "RLC");
            break;
        case 0x09: // DAD
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "ADD HL, BC\t; DAD rp");
            break;
        case 0x0A: 
            // LDAX (BC)
            cpu->a = cpu->motherboard_memory[GET_BC];
            break;
        case 0x0B: // DCX
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "DEC BC\t; DCX rp");
            break;
        case 0x0C: // INR
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "INC C\t; INR r");
            break;
        case 0x0D: // DCR
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "DEC C\t; DCR r");
            break;
        case 0x0E: 
            // MVI C, d8  (LD C, d8)
            cpu->c = cpu->motherboard_memory[cpu->pc + 1];
            pc_increments = 2;
            break;
        case 0x0F: // RRC
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "RRCA");
            break;
        case 0x11:
            // LXI DE, data 16
            cpu->d = cpu->motherboard_memory[cpu->pc + 2];
            cpu->e = cpu->motherboard_memory[cpu->pc + 1];
            pc_increments = 3;
            break;
        case 0x12: // STAX
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "STAX (DE)");
            break;
        case 0x13: 
            // INC DE (INX rp)
            tmp_rp = GET_DE;
            tmp_rp++;
            cpu->d = (tmp_rp & 0xFF00) >> 8;
            cpu->e = (tmp_rp & 0x00FF);
            break;
        case 0x14: // INR
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "INC D\t; INR r");
            break;
        case 0x15: // DCR
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "DEC D\t; DCR r");
            break;
        case 0x16: 
            // MVI D, d8  (LD D, d8)
            cpu->d = cpu->motherboard_memory[cpu->pc + 1];
            pc_increments = 2;
            break;
        case 0x17: // RAL
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "RAL");
            break;
        case 0x19: // DAD
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "ADD HL, DE\t; DAD rp");
            break;
        case 0x1A: 
            // LDAX DE
            cpu->a = cpu->motherboard_memory[GET_DE];
            break;
        case 0x1B: // DCX
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "DEC DE\t; DCX rp");
            break;
        case 0x1C: // INR
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "INC E\t; INR r");
            break;
        case 0x1D: // DCR
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "DEC E\t; DCR r");
            break;
        case 0x1E: 
            // MVI E, d8  (LD E, d8)
            cpu->e = cpu->motherboard_memory[cpu->pc + 1];
            pc_increments = 2;
            break;
        case 0x1F: // RAR
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "RAR");
            break;
        case 0x21: 
            // LXI HL, data 16
            cpu->h = cpu->motherboard_memory[cpu->pc + 2];
            cpu->l = cpu->motherboard_memory[cpu->pc + 1];
            pc_increments = 3;
            break;
        case 0x22: // SHLD
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "LD ($%02X%02X), HL\t; SHLD", memory[addr+2], memory[addr+1]);
            // retval = 3;
            break;
        case 0x23: // INX
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "INC HL\t; INX rp");
            break;
        case 0x24: // INR
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "INC H\t; INR r");
            break;
        case 0x25: // DCR
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "DEC H\t; DCR r");
            break;
        case 0x26: 
            // MVI H, d8  (LD H, d8)
            cpu->h = cpu->motherboard_memory[cpu->pc + 1];
            pc_increments = 2;
            break;
        case 0x27: // DAA
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "DAA");
            break;
        case 0x29: // DAD
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "ADD HL, HL\t; DAD rp");
            break;
        case 0x2A: // LHLD
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "LD HL, ($%02X%02X)\t; LHLD", memory[addr+2], memory[addr+1]);
            // retval = 3;
            break;
        case 0x2B: // DCX
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "DEC HL\t; DCX rp");
            break;
        case 0x2C: // INR
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "INC L\t; INR r");
            break;
        case 0x2D: // DCR
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "DEC L\t; DCR r");
            break;
        case 0x2E: 
            // MVI L, d8  (LD L, d8)
            cpu->l = cpu->motherboard_memory[cpu->pc + 1];
            pc_increments = 2;
            break;
        case 0x2F: // CMA
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "CMA");
            break;
        case 0x31:
            // LXI SP, data 16
            cpu->sp = (cpu->motherboard_memory[cpu->pc + 2] << 8) | cpu->motherboard_memory[cpu->pc + 1];
            cpu->stack_pointer_start = cpu->sp;
            pc_increments = 3;
            break;
        case 0x32: // STA
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "LD ($%02X%02X), A\t; STA", memory[addr+2], memory[addr+1]);
            // retval = 3;
            break;
        case 0x33: // INX
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "INC SP\t; INX rp");
            break;
        case 0x34: // INR
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "INC (HL)\t; INR M");
            break;
        case 0x35: // DCR
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "DEC (HL)\t; DCR M");
            break;
        case 0x36: 
            // MVI M, data (a.k.a. LD (HL), data)
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "LD (HL), $%02X\t;MVI M, data", memory[addr+1]);
            /*
            cpu->motherboard_memory[GET_HL] = cpu->motherboard_memory[cpu->pc + 1];
            */
            break;
        case 0x37: // STC
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "STC");
            break;
        case 0x39: // DAD
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "ADD HL, SP\t; DAD rp");
            break;
        case 0x3A: // LDA
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "LD A, ($%02X%02X)\t; LDA", memory[addr+2], memory[addr+1]);
            // retval = 3;
            break;
        case 0x3B: // DCX
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "DEC SP\t; DCX rp");
            break;
        case 0x3C: // INR
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "INC A\t; INR r");
            break;
        case 0x3D: // DCR
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "DEC A\t; DCR r");
            break;
        case 0x3E:
            // MVI A, d8  (LD A, d8)
            cpu->a = cpu->motherboard_memory[cpu->pc + 1];
            pc_increments = 2;
            break;
        case 0x3F: // CMC
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "CMC");
            break;
        case 0x40: // MOV
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "LD B, B\t; MOV r1, r2");
            break;
        case 0x41: // MOV
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "LD B, C\t; MOV r1, r2");
            break;
        case 0x42: // MOV
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "LD B, D\t; MOV r1, r2");
            break;
        case 0x43: // MOV
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "LD B, E\t; MOV r1, r2");
            break;
        case 0x44: // MOV
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "LD B, H\t; MOV r1, r2");
            break;
        case 0x45: // MOV
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "LD B, L\t; MOV r1, r2");
            break;
        case 0x46: // MOV
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "LD B, (HL)\t; MOV r, M");
            break;
        case 0x47: // MOV
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "LD B, A\t; MOV r1, r2");
            break;
        case 0x48: // MOV
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "LD C, B\t; MOV r1, r2");
            break;
        case 0x49: // MOV
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "LD C, C\t; MOV r1, r2");
            break;
        case 0x4A: // MOV
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "LD C, D\t; MOV r1, r2");
            break;
        case 0x4B: // MOV
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "LD C, E\t; MOV r1, r2");
            break;
        case 0x4C: // MOV
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "LD C, H\t; MOV r1, r2");
            break;
        case 0x4D: // MOV
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "LD C, L\t; MOV r1, r2");
            break;
        case 0x4E: // MOV
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "LD C, (HL)\t; MOV r, M");
            break;
        case 0x4F: // MOV
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "LD C, A\t; MOV r1, r2");
            break;
        case 0x50: // MOV
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "LD D, B\t; MOV r1, r2");
            break;
        case 0x51: // MOV
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "LD D, C\t; MOV r1, r2");
            break;
        case 0x52: // MOV
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "LD D, D\t; MOV r1, r2");
            break;
        case 0x53: // MOV
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "LD D, E\t; MOV r1, r2");
            break;
        case 0x54: // MOV
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "LD D, H\t; MOV r1, r2");
            break;
        case 0x55: // MOV
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "LD D, L\t; MOV r1, r2");
            break;
        case 0x56: // MOV
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "LD D, (HL)\t; MOV r, M");
            break;
        case 0x57: // MOV
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "LD D, A\t; MOV r1, r2");
            break;
        case 0x58: // MOV
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "LD E, B\t; MOV r1, r2");
            break;
        case 0x59: // MOV
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "LD E, C\t; MOV r1, r2");
            break;
        case 0x5A: // MOV
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "LD E, D\t; MOV r1, r2");
            break;
        case 0x5B: // MOV
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "LD E, E\t; MOV r1, r2");
            break;
        case 0x5C: // MOV
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "LD E, H\t; MOV r1, r2");
            break;
        case 0x5D: // MOV
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "LD E, L\t; MOV r1, r2");
            break;
        case 0x5E: // MOV
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "LD E, (HL)\t; MOV r, M");
            break;
        case 0x5F: // MOV
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "LD E, A\t; MOV r1, r2");
            break;
        case 0x60: // MOV
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "LD H, B\t; MOV r1, r2");
            break;
        case 0x61: // MOV
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "LD H, C\t; MOV r1, r2");
            break;
        case 0x62: // MOV
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "LD H, D\t; MOV r1, r2");
            break;
        case 0x63: // MOV
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "LD H, E\t; MOV r1, r2");
            break;
        case 0x64: // MOV
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "LD H, H\t; MOV r1, r2");
            break;
        case 0x65: // MOV
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "LD H, L\t; MOV r1, r2");
            break;
        case 0x66: // MOV
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "LD H, (HL)\t; MOV r, M");
            break;
        case 0x67: // MOV
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "LD H, A\t; MOV r1, r2");
            break;
        case 0x68: // MOV
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "LD L, B\t; MOV r1, r2");
            break;
        case 0x69: // MOV
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "LD L, C\t; MOV r1, r2");
            break;
        case 0x6A: // MOV
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "LD L, D\t; MOV r1, r2");
            break;
        case 0x6B: // MOV
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "LD L, E\t; MOV r1, r2");
            break;
        case 0x6C: // MOV
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "LD L, H\t; MOV r1, r2");
            break;
        case 0x6D: // MOV
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "LD L, L\t; MOV r1, r2");
            break;
        case 0x6E: // MOV
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "LD L, (HL)\t; MOV r, M");
            break;
        case 0x6F: // MOV
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "LD L, A\t; MOV r1, r2");
            break;
        case 0x70: // MOV
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "LD (HL), B\t; MOV M, r");
            break;
        case 0x71: // MOV
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "LD (HL), C\t; MOV M, r");
            break;
        case 0x72: // MOV
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "LD (HL), D\t; MOV M, r");
            break;
        case 0x73: // MOV
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "LD (HL), E\t; MOV M, r");
            break;
        case 0x74: // MOV
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "LD (HL), H\t; MOV M, r");
            break;
        case 0x75: // MOV
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "LD (HL), L\t; MOV M, r");
            break;
        case 0x76: // HLT
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "HLT");
            break;
        case 0x77: // MOV
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "LD (HL), A\t; MOV M, r");
            break;
        case 0x78: // MOV
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "LD A, B\t; MOV r1, r2");
            break;
        case 0x79: // MOV
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "LD A, C\t; MOV r1, r2");
            break;
        case 0x7A: // MOV
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "LD A, D\t; MOV r1, r2");
            break;
        case 0x7B: // MOV
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "LD A, E\t; MOV r1, r2");
            break;
        case 0x7C: // MOV
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "LD A, H\t; MOV r1, r2");
            break;
        case 0x7D: // MOV
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "LD A, L\t; MOV r1, r2");
            break;
        case 0x7E: // MOV
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "LD A, (HL)\t; MOV r, M");
            break;
        case 0x7F: // MOV
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "LD A, A\t; MOV r1, r2");
            break;
        case 0x80: 
            // ADD A, B
            do_addition(cpu, cpu->b, false);
            break;
        case 0x81: 
            // ADD, C
            do_addition(cpu, cpu->c, false);
            break;
        case 0x82: 
            // ADD, D
            do_addition(cpu, cpu->d, false);
            break;
        case 0x83: 
            // ADD, E
            do_addition(cpu, cpu->e, false);
            break;
        case 0x84: 
            // ADD, C
            do_addition(cpu, cpu->h, false);
            break;
        case 0x85: 
            // ADD, C
            do_addition(cpu, cpu->l, false);
            break;
        case 0x86: 
            // ADD, (HL))
            do_addition(cpu, cpu->motherboard_memory[GET_HL], false);
            break;
        case 0x87: 
            // ADD, A
            do_addition(cpu, cpu->a, false);
            break;
        case 0x88: 
            // ADC A, B
            do_addition(cpu, cpu->b, cpu->carry_flag);
            break;
        case 0x89: 
            // ADC A, C
            do_addition(cpu, cpu->c, cpu->carry_flag);
            break;
        case 0x8A: 
            // ADC A, D
            do_addition(cpu, cpu->d, cpu->carry_flag);
            break;
        case 0x8B: 
            // ADC A, E
            do_addition(cpu, cpu->e, cpu->carry_flag);
            break;
        case 0x8C: 
            // ADC A, H
            do_addition(cpu, cpu->h, cpu->carry_flag);
            break;
        case 0x8D: 
            // ADC A, L
            do_addition(cpu, cpu->l, cpu->carry_flag);
            break;
        case 0x8E: 
            // ADC, (HL))
            do_addition(cpu, cpu->motherboard_memory[GET_HL], cpu->carry_flag);
            break;
        case 0x8F: 
            // ADC A, A
            do_addition(cpu, cpu->a, cpu->carry_flag);
            break;
        case 0x90: 
            //SUB A, B
            do_subtraction(cpu, cpu->b, false, true);
            break;
        case 0x91: 
            //SUB A, C
            do_subtraction(cpu, cpu->c, false, true);
            break;
        case 0x92: 
            //SUB A, D
            do_subtraction(cpu, cpu->d, false, true);
            break;
        case 0x93: 
            //SUB A, E
            do_subtraction(cpu, cpu->e, false, true);
            break;
        case 0x94: 
            //SUB A, H
            do_subtraction(cpu, cpu->h, false, true);
            break;
        case 0x95: 
            //SUB A, L
            do_subtraction(cpu, cpu->l, false, true);
            break;
        case 0x96: 
            //SUB A, (HL)
            do_subtraction(cpu, cpu->motherboard_memory[GET_HL], false, true);
            break;
        case 0x97: 
            //SUB A, A
            do_subtraction(cpu, cpu->a, false, true);
            break;
        case 0x98:
            // SBB A, B
            do_subtraction(cpu, cpu->b, cpu->carry_flag, true);
            break;
        case 0x99: 
            // SBB A, C
            do_subtraction(cpu, cpu->c, cpu->carry_flag, true);
            break;
        case 0x9A: 
            // SBB A, D
            do_subtraction(cpu, cpu->d, cpu->carry_flag, true);
            break;
        case 0x9B: 
            // SBB A, E
            do_subtraction(cpu, cpu->e, cpu->carry_flag, true);
            break;
        case 0x9C: 
            // SBB A, H
            do_subtraction(cpu, cpu->h, cpu->carry_flag, true);
            break;
        case 0x9D: 
            // SBB A, L
            do_subtraction(cpu, cpu->l, cpu->carry_flag, true);
            break;
        case 0x9E: 
            // SBB A, (HL)
            do_subtraction(cpu, cpu->motherboard_memory[GET_HL], cpu->carry_flag, true);
            break;
        case 0x9F: 
            // SBB A, A
            do_subtraction(cpu, cpu->a, cpu->carry_flag, true);
            break;
        case 0xA0: 
            // AND B (ANA r)
            do_and(cpu, cpu->b);
            break;
        case 0xA1: 
            // AND C (ANA r)
            do_and(cpu, cpu->c);
            break;
        case 0xA2: 
            // AND D (ANA r)
            do_and(cpu, cpu->d);
            break;
        case 0xA3: 
            // AND E (ANA r)
            do_and(cpu, cpu->e);
            break;
        case 0xA4: 
            // AND H (ANA r)
            do_and(cpu, cpu->h);
            break;
        case 0xA5: 
            // AND L (ANA r)
            do_and(cpu, cpu->l);
            break;
        case 0xA6:
            // AND (HL) (ANA M)
            do_and(cpu, cpu->motherboard_memory[GET_HL]);
            break; 
        case 0xA7: 
            // AND A (ANA r)
            do_and(cpu, cpu->a);
            break;
        case 0xA8: 
            // XOR B (XRA r)
            do_xor(cpu, cpu->b);
            break;
        case 0xA9: 
            // XOR C (XRA r)
            do_xor(cpu, cpu->c);
            break;
        case 0xAA: 
            // XOR D (XRA r)
            do_xor(cpu, cpu->d);
            break;
        case 0xAB: 
            // XOR E (XRA r)
            do_xor(cpu, cpu->e);
            break;
        case 0xAC: 
            // XOR H (XRA r)
            do_xor(cpu, cpu->h);
            break;
        case 0xAD: 
            // XOR L (XRA r)
            do_xor(cpu, cpu->l);
            break;
        case 0xAE: 
            // XOR (HL) (XRA M)
            do_xor(cpu, cpu->motherboard_memory[GET_HL]);
            break;
        case 0xAF: 
            // XOR A (XRA r)
            do_xor(cpu, cpu->a);
            break;
        case 0xB0: 
            // OR B (ORA r)
            do_or(cpu, cpu->b); 
            break;
        case 0xB1: 
            // OR C (ORA r)
            do_or(cpu, cpu->c); 
            break;
        case 0xB2: 
            // OR D (ORA r)
            do_or(cpu, cpu->d); 
            break;
        case 0xB3: 
            // OR E (ORA r)
            do_or(cpu, cpu->e); 
            break;
        case 0xB4: 
            // OR H (ORA r)
            do_or(cpu, cpu->h); 
            break;
        case 0xB5: 
            // OR L (ORA r)
            do_or(cpu, cpu->l); 
            break;
        case 0xB6: 
            // OR (HL) (ORA M)
            do_or(cpu, cpu->motherboard_memory[GET_HL]); 
            break;
        case 0xB7: 
            // OR A (ORA r)
            do_or(cpu, cpu->a); 
            break;
        case 0xB8: 
            // CMP B
            do_subtraction(cpu, cpu->b, false, false);
            break;
        case 0xB9: 
            // CMP C
            do_subtraction(cpu, cpu->c, false, false);
            break;
        case 0xBA: 
            // CMP D
            do_subtraction(cpu, cpu->d, false, false);
            break;
        case 0xBB: 
            // CMP E
            do_subtraction(cpu, cpu->e, false, false);
            break;
        case 0xBC: 
            // CMP H
            do_subtraction(cpu, cpu->h, false, false);
            break;
        case 0xBD: 
            // CMP L
            do_subtraction(cpu, cpu->l, false, false);
            break;
        case 0xBE: 
            // CMP M (CMP (HL))
            do_subtraction(cpu, cpu->motherboard_memory[GET_HL], false, false);
            break;
        case 0xBF: 
            // CMP A
            do_subtraction(cpu, cpu->a, false, false);
            break;
        case 0xC0: 
            // RET NZ
            do_conditional_return(cpu, !(cpu->zero_flag), num_states, &pc_increments);
            break;
        case 0xC1: // POP
            // POP BC
            cpu->b = cpu->motherboard_memory[cpu->sp + 1];
            cpu->c = cpu->motherboard_memory[cpu->sp];
            cpu->sp = cpu->sp + 2;
            break;
        case 0xC2: 
            // JMP NZ
            do_conditional_jump(cpu, !(cpu->zero_flag), &pc_increments);
            break;
        case 0xC3: // JMP
            cpu->pc = (cpu->motherboard_memory[cpu->pc+2] << 8) | cpu->motherboard_memory[cpu->pc+1];
            pc_increments = 0;
            break;
        case 0xC4:
            // CALL NZ addr
            do_conditional_call(cpu, !(cpu->zero_flag), num_states, &pc_increments);
            break;
        case 0xC5:
            // PUSH BC
            cpu->motherboard_memory[cpu->sp - 1] = cpu->b;
            cpu->motherboard_memory[cpu->sp - 2] = cpu->c;
            cpu->sp = cpu->sp - 2;
        case 0xC6: 
            // ADI d8
            do_addition(cpu, cpu->motherboard_memory[cpu->pc + 1], false);
            pc_increments = 2;
            break;
        case 0xC7: // RST
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "RST 000");
            break;
        case 0xC8: 
            // RET Z
            do_conditional_return(cpu, cpu->zero_flag, num_states, &pc_increments);
            break;
        case 0xC9: 
            // RET
            do_conditional_return(cpu, true, num_states, &pc_increments);
            // conditional returns take 11 states if the condition is true.  This is an unconditional return
            // which only takes 10.  I'll pull it from the table though, as the compiler should make this a static assignment anyway, 
            // and this way if the table updates later for a different CPU I don't need to remember to change this code.
            *num_states = states_per_opcode[0xC9];
            break;
        case 0xCA:
            // JMP Z
            do_conditional_jump(cpu, cpu->zero_flag, &pc_increments);
            break;
        case 0xCC: 
            // CALL Z addr
            do_conditional_call(cpu, cpu->zero_flag, num_states, &pc_increments);
            break;
        case 0xCD: 
            // CALL addr
            // this is an unconditional call, so pass in true for the flag
            do_conditional_call(cpu, true, num_states, &pc_increments);
            break;
        case 0xCE: 
            // ACI data (add immediate with carry);
            do_addition(cpu, cpu->motherboard_memory[cpu->pc + 1], cpu->carry_flag);
            pc_increments = 2;
            break;
        case 0xCF: // RST
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "RST 001");
            break;
        case 0xD0: 
            // RET NC
            do_conditional_return(cpu, !(cpu->carry_flag), num_states, &pc_increments);
            break;
        case 0xD1:
            // POP DE
            cpu->d = cpu->motherboard_memory[cpu->sp + 1];
            cpu->e = cpu->motherboard_memory[cpu->sp];
            cpu->sp = cpu->sp + 2;
            break;
        case 0xD2: 
            // JMP NC
            do_conditional_jump(cpu, !(cpu->carry_flag), &pc_increments);
            break;
        case 0xD3: 
            // OUT port, A
            cpu->motherboard_output_handler(cpu->motherboard_memory[cpu->pc + 1], cpu->a);
            pc_increments = 2;
            break;
        case 0xD4: 
            // CALL NC addr
            do_conditional_call(cpu, !(cpu->carry_flag), num_states, &pc_increments);
            break;
        case 0xD5: 
            // PUSH DE
            cpu->motherboard_memory[cpu->sp - 1] = cpu->d;
            cpu->motherboard_memory[cpu->sp - 2] = cpu->e;
            cpu->sp = cpu->sp - 2;
            break;
        case 0xD6: 
            // SUI data
            do_subtraction(cpu, cpu->motherboard_memory[cpu->pc + 1], false, true);
            pc_increments = 2;
            break;
        case 0xD7: // RST
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "RST 002");
            break;
        case 0xD8: 
            // RET C
            do_conditional_return(cpu, cpu->carry_flag, num_states, &pc_increments);
            break;
        case 0xDA:
            // JMP C
            do_conditional_jump(cpu, cpu->carry_flag, &pc_increments);
            break;
        case 0xDB: // IN
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "IN A, (INP %02X)", memory[addr+1]);
            // retval = 2;
            break;
        case 0xDC: 
            // CALL C addr
            do_conditional_call(cpu, cpu->carry_flag, num_states, &pc_increments);
            break;
        case 0xDE: 
            // SBI data (subtract intermediate with carry)
            do_subtraction(cpu, cpu->motherboard_memory[cpu->pc + 1], cpu->carry_flag, true);
            pc_increments = 2;
            break;
        case 0xDF: // RST
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "RST 003");
            break;
        case 0xE0: 
            // RET PO
            do_conditional_return(cpu, !(cpu->parity_flag), num_states, &pc_increments);
            break;
        case 0xE1: // POP
            // POP HL
            cpu->h = cpu->motherboard_memory[cpu->sp + 1];
            cpu->l = cpu->motherboard_memory[cpu->sp];
            cpu->sp = cpu->sp + 2;
            break;
        case 0xE2: 
            // JMP PO
            do_conditional_jump(cpu, !(cpu->parity_flag), &pc_increments);
            break;
        case 0xE3: // XTHL
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "XTHL");
            break;
        case 0xE4: 
            // CALL PO addr
            do_conditional_call(cpu, !(cpu->parity_flag), num_states, &pc_increments);
            break;
        case 0xE5:
            // PUSH HL
            cpu->motherboard_memory[cpu->sp - 1] = cpu->h;
            cpu->motherboard_memory[cpu->sp - 2] = cpu->l;
            cpu->sp = cpu->sp - 2;
        case 0xE6: 
            // AND data (ANI data)
            do_and(cpu, cpu->motherboard_memory[cpu->pc + 1]);
            pc_increments = 2;
            break;
        case 0xE7: // RST
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "RST 004");
            break;
        case 0xE8: 
            // RET PE
            do_conditional_return(cpu, cpu->parity_flag, num_states, &pc_increments);
            break;
        case 0xE9: // PCHL
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "PCHL");
            break;
        case 0xEA:
            // JMP PE
            do_conditional_jump(cpu, cpu->parity_flag, &pc_increments);
            break;
        case 0xEB: 
            // XCHG (a.k.a. EX DE, HL)
            tmp_d = cpu->d;
            tmp_e = cpu->e;
            cpu->d = cpu->h;
            cpu->e = cpu->l;
            cpu->h = tmp_d;
            cpu->l = tmp_e;
            break;
        case 0xEC: 
            // CALL PE addr
            do_conditional_call(cpu, cpu->parity_flag, num_states, &pc_increments);
            break;
        case 0xEE: 
            // XRI data (XOR intermediate)
            do_xor(cpu, cpu->motherboard_memory[cpu->pc + 1]);
            pc_increments = 2;
            break;
        case 0xEF: // RST
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "RST 005");
            break;
        case 0xF0: 
            // RET P
            do_conditional_return(cpu, !(cpu->sign_flag), num_states, &pc_increments);
            break;
        case 0xF1: 
            // POP PSW (POP AF)
            cpu->a = cpu->motherboard_memory[cpu->sp + 1];
            set_flags_from_byte(cpu, cpu->motherboard_memory[cpu->sp]);
            cpu->sp = cpu->sp + 2;
            break;
        case 0xF2: 
            // JMP P
            do_conditional_jump(cpu, !(cpu->sign_flag), &pc_increments);
            break;
        case 0xF3: // DI
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "DI");
            break;
        case 0xF4: 
            // CALL P addr
            do_conditional_call(cpu, !(cpu->sign_flag), num_states, &pc_increments);
            break;
        case 0xF5:
            // PUSH PSW (a.k.a. PUSH AF)
            cpu->motherboard_memory[cpu->sp - 1] = cpu->a;
            cpu->motherboard_memory[cpu->sp - 2] = get_byte_from_flags(cpu);
            cpu->sp = cpu->sp - 2;
        case 0xF6: 
            // OR data (ORI data)
            do_or(cpu, cpu->motherboard_memory[cpu->pc + 1]);
            pc_increments = 2;
            break;
        case 0xF7: // RST
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "RST 006");
            break;
        case 0xF8: 
            // RET M
            do_conditional_return(cpu, cpu->sign_flag, num_states, &pc_increments);
            break;
        case 0xF9: // SPHL
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "SPHL");
            break;
        case 0xFA:
            // JMP M
            do_conditional_jump(cpu, cpu->sign_flag, &pc_increments);
            break;
        case 0xFB: // EI
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "EI");
            break;
        case 0xFC: 
            // CALL M addr
            do_conditional_call(cpu, cpu->sign_flag, num_states, &pc_increments);
            break;
        case 0xFE: 
            // CPI data (CMP data)
            do_subtraction(cpu, cpu->motherboard_memory[cpu->pc + 1], false, false);
            pc_increments = 2;
            break;
        case 0xFF: // RST
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "RST 007");
            break;
        default:  // 0x08, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38, 0xCB, 0xD9, 0xDD, 0xED, 0xFD
            printf("ERROR: Opcode %02X not valid\n", opcode); return (false); //(strbuff, "??");
    }
    cpu->pc = cpu->pc + pc_increments;
    return true;
}

// cycle() returns false on error
bool cycle_cpu8080(cpu8080 *cpu, uint64_t *num_states) {
    bool flip_interrupts_on, retval;
    if (!(cpu->halted)){
        flip_interrupts_on = cpu->enable_interrupts_after_next_instruction;
        retval = do_opcode(cpu, num_states);
        if (flip_interrupts_on){
            cpu->interrupts_enabled = true;
            cpu->enable_interrupts_after_next_instruction = false;
        }
        return(retval);
    }
    else {
        *num_states = 0;
        return true;
    }
}