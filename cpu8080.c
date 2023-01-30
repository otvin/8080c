#include <stdio.h>
#include "cpu8080.h"

#define GET_BC (((cpu->b) << 8) | (cpu->c))
#define GET_DE (((cpu->d) << 8) | (cpu->e))
#define GET_HL (((cpu->h) << 8) | (cpu->l))
#define DATA_TO_INT16 ((cpu->motherboard_memory[cpu->pc + 2] << 8) | cpu->motherboard_memory[cpu->pc + 1])

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
    uint32_t tmp_32; // used in opcodes like DAD where we operate on two 16-bit numbers and need to see if there is a carry.
    uint8_t tmp_16; // used in DAA
    
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
        case 0x02: 
            // STAX (BC)
            cpu->motherboard_memory[GET_BC] = cpu->a;
            break;
        case 0x03: // INX
            // INC BC (INX rp)
            tmp_rp = GET_BC;
            tmp_rp++;
            cpu->b = (tmp_rp & 0xFF00) >> 8;
            cpu->c = (tmp_rp & 0x00FF);
            break;
        case 0x04: 
            // INC B (INR r)
            cpu->b ++;
            set_zero_sign_parity_from_byte(cpu, cpu->b);
            cpu->auxiliary_carry_flag = (bool)(((cpu->b) & 0x0F) == 0);
            break;
        case 0x05:
            // DEC B (DCR r)
            cpu->b --;
            set_zero_sign_parity_from_byte(cpu, cpu->b);
            cpu->auxiliary_carry_flag = (bool)(((cpu->b) & 0x0F) != 0x0F);
            break;
        case 0x06: 
            // MVI B, d8  (LD B, d8)
            cpu->b = cpu->motherboard_memory[cpu->pc + 1];
            pc_increments = 2;
            break;
        case 0x07: // RLC
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "RLC");
            break;
        case 0x09: 
            // ADD HL, BC (DAD rp)
            // Only the carry flag is affected, and then if the 16-bit addition carries out.
            tmp_32 = (uint32_t)(GET_HL) + (uint32_t)(GET_BC);
            cpu->carry_flag = (bool)(tmp_32 > 0xFFFF);
            cpu->h = (uint8_t)((tmp_32 & 0xFF00) >> 8);
            cpu->l = (uint8_t)(tmp_32 & 0x00FF);
            break;
        case 0x0A: 
            // LDAX (BC)
            cpu->a = cpu->motherboard_memory[GET_BC];
            break;
        case 0x0B: 
            // DEC BC (INX rp)
            tmp_rp = GET_BC;
            tmp_rp--;
            cpu->b = (tmp_rp & 0xFF00) >> 8;
            cpu->c = (tmp_rp & 0x00FF);
            break;
        case 0x0C: 
            // INC C (INR r)
            cpu->c ++;
            set_zero_sign_parity_from_byte(cpu, cpu->c);
            cpu->auxiliary_carry_flag = (bool)(((cpu->c) & 0x0F) == 0);
            break;
        case 0x0D: 
            // DEC C (DCR r)
            cpu->c --;
            set_zero_sign_parity_from_byte(cpu, cpu->c);
            cpu->auxiliary_carry_flag = (bool)(((cpu->c) & 0x0F) != 0x0F);
            break;
        case 0x0E: 
            // MVI C, d8  (LD C, d8)
            cpu->c = cpu->motherboard_memory[cpu->pc + 1];
            pc_increments = 2;
            break;
        case 0x0F: 
            // RRCA (Rotate Right with Carry)
            // The high order bit and the CY flag are both set to the value shifted out of the low order bit position. 
            // Only the CY flag is affected.
            cpu->carry_flag = (bool)(cpu->a & 0x01);
            cpu->a = cpu->a >> 1;
            if (cpu->carry_flag) {
                cpu->a |= 0x80;
            }
            break;
        case 0x11:
            // LXI DE, data 16
            cpu->d = cpu->motherboard_memory[cpu->pc + 2];
            cpu->e = cpu->motherboard_memory[cpu->pc + 1];
            pc_increments = 3;
            break;
        case 0x12: 
            // STAX (DE)
            cpu->motherboard_memory[GET_DE] = cpu->a;
            break;
        case 0x13: 
            // INC DE (INX rp)
            tmp_rp = GET_DE;
            tmp_rp++;
            cpu->d = (tmp_rp & 0xFF00) >> 8;
            cpu->e = (tmp_rp & 0x00FF);
            break;
        case 0x14: 
            // INC D (INR r)
            cpu->d ++;
            set_zero_sign_parity_from_byte(cpu, cpu->d);
            cpu->auxiliary_carry_flag = (bool)(((cpu->d) & 0x0F) == 0);
            break;
        case 0x15: 
            // DEC D (DCR r)
            cpu->d --;
            set_zero_sign_parity_from_byte(cpu, cpu->d);
            cpu->auxiliary_carry_flag = (bool)(((cpu->d) & 0x0F) != 0x0F);
            break;
        case 0x16: 
            // MVI D, d8  (LD D, d8)
            cpu->d = cpu->motherboard_memory[cpu->pc + 1];
            pc_increments = 2;
            break;
        case 0x17: // RAL
            printf("ERROR: Opcode %02X not implemented\n", opcode); return (false); //(strbuff, "RAL");
            break;
        case 0x19: 
            // ADD HL, DE (DAD rp)
            // Only the carry flag is affected, and then if the 16-bit addition carries out.
            tmp_32 = (uint32_t)(GET_HL) + (uint32_t)(GET_DE);
            cpu->carry_flag = (bool)(tmp_32 > 0xFFFF);
            cpu->h = (uint8_t)((tmp_32 & 0xFF00) >> 8);
            cpu->l = (uint8_t)(tmp_32 & 0x00FF);
            break;
        case 0x1A: 
            // LDAX DE
            cpu->a = cpu->motherboard_memory[GET_DE];
            break;
        case 0x1B: 
            // DEC DE (INX rp)
            tmp_rp = GET_DE;
            tmp_rp--;
            cpu->d = (tmp_rp & 0xFF00) >> 8;
            cpu->e = (tmp_rp & 0x00FF);
            break;
        case 0x1C: 
            // INC E (INR r)
            cpu->e ++;
            set_zero_sign_parity_from_byte(cpu, cpu->e);
            cpu->auxiliary_carry_flag = (bool)(((cpu->e) & 0x0F) == 0);
            break;
        case 0x1D: 
            // DEC E (DCR r)
            cpu->e --;
            set_zero_sign_parity_from_byte(cpu, cpu->e);
            cpu->auxiliary_carry_flag = (bool)(((cpu->e) & 0x0F) != 0x0F);
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
        case 0x22: 
            // SHLD addr  (store H and L direct)
            // The content of register L is moved to teh memory location whose address is specified in byte 2 and
            // byte 3.  The content of register H is moved to the succeeding memory location.  Flags are not 
            // affected.
            cpu->motherboard_memory[DATA_TO_INT16] = cpu->l;
            cpu->motherboard_memory[DATA_TO_INT16 + 1] = cpu->h;
            pc_increments = 3;
            break;
        case 0x23: 
            // INC HL (INX rp)
            tmp_rp = GET_HL;
            tmp_rp++;
            cpu->h = (tmp_rp & 0xFF00) >> 8;
            cpu->l = (tmp_rp & 0x00FF);
            break;
        case 0x24: 
            // INC H (INR r)
            cpu->h ++;
            set_zero_sign_parity_from_byte(cpu, cpu->h);
            cpu->auxiliary_carry_flag = (bool)(((cpu->h) & 0x0F) == 0);
            break;
        case 0x25: 
            // DEC H (DCR r)
            cpu->h --;
            set_zero_sign_parity_from_byte(cpu, cpu->h);
            cpu->auxiliary_carry_flag = (bool)(((cpu->h) & 0x0F) != 0x0F);
            break;
        case 0x26: 
            // MVI H, d8  (LD H, d8)
            cpu->h = cpu->motherboard_memory[cpu->pc + 1];
            pc_increments = 2;
            break;
        case 0x27: 
            // DAA (Decimal adjust accumulator)
            /* The eight-bit number in the accumulator is adjusted to form two four-bit Binary-Coded-Decimal digits
               by the following process:
               1. If the value of the least significant 4 bits of the accumulator is greater than 9 or if the AC flag
               is set, 6 is added to the accumulator.
               2. If the value of the most significant 4 bits of the accumulator is now greater than 9, or if the CY
               flag is set, 6 is added to the most significant 4 bits of the accumulator.
               NOTE: All flags are affected */
        
            // copying the MAME logic
            tmp_16 = cpu->a;  // TODO could probably use a tmp_8 here but this works for now.
            if (cpu->auxiliary_carry_flag || ((cpu->a & 0xF) > 0x9)) {
                tmp_16 = tmp_16 + 0x06;
            }
            if (cpu->carry_flag || (cpu->a > 0x99)) {
                tmp_16 = tmp_16 + 0x60;
            }
            cpu->carry_flag = (bool)(cpu->carry_flag || (cpu->a > 0x99));
            cpu->auxiliary_carry_flag = (bool)((cpu->a | tmp_16) & 0x10);
            cpu->a = (uint8_t)(tmp_16 & 0xFF);
            set_zero_sign_parity_from_byte(cpu, cpu->a);
            break;
        case 0x29: // DAD
            // ADD HL, HL (DAD rp)
            // Only the carry flag is affected, and then if the 16-bit addition carries out.
            // TODO: This could be made faster by converting this to a shift left by 1 (because HL + HL == 2 * HL == HL << 1)
            tmp_32 = (uint32_t)(GET_HL) + (uint32_t)(GET_HL);
            cpu->carry_flag = (bool)(tmp_32 > 0xFFFF);
            cpu->h = (uint8_t)((tmp_32 & 0xFF00) >> 8);
            cpu->l = (uint8_t)(tmp_32 & 0x00FF);
            break;
        case 0x2A: 
            // LHLD addr (load H and L direct)
            // The content of the memory location is specified in byte 2 and byte 3 of the instruction, is moved to
            // register L.  The content of the memory location at the succeeding address is moved to register H.
            // flags are not affected.
            cpu->l = cpu->motherboard_memory[DATA_TO_INT16];
            cpu->h = cpu->motherboard_memory[DATA_TO_INT16 + 1];
            pc_increments = 3;
            break;
        case 0x2B: 
            // DEC HL (INX rp)
            tmp_rp = GET_HL;
            tmp_rp--;
            cpu->h = (tmp_rp & 0xFF00) >> 8;
            cpu->l = (tmp_rp & 0x00FF);
            break;
        case 0x2C: 
            // INC L (INR r)
            cpu->l ++;
            set_zero_sign_parity_from_byte(cpu, cpu->l);
            cpu->auxiliary_carry_flag = (bool)(((cpu->l) & 0x0F) == 0);
            break;
        case 0x2D:
            // DEC L (DCR r)
            cpu->l --;
            set_zero_sign_parity_from_byte(cpu, cpu->l);
            cpu->auxiliary_carry_flag = (bool)(((cpu->l) & 0x0F) != 0x0F);
            break;
        case 0x2E: 
            // MVI L, d8  (LD L, d8)
            cpu->l = cpu->motherboard_memory[cpu->pc + 1];
            pc_increments = 2;
            break;
        case 0x2F: 
            // CMA (complement accumulator)
            cpu->a = ~(cpu->a);
            break;
        case 0x31:
            // LXI SP, data 16
            cpu->sp = DATA_TO_INT16;
            cpu->stack_pointer_start = cpu->sp;
            pc_increments = 3;
            break;
        case 0x32: 
            // STA data 16 (store accumulator direct)
            cpu->motherboard_memory[DATA_TO_INT16] = cpu->a;
            pc_increments = 3;
            break;
        case 0x33: 
            // INC SP (INX rp)
            cpu->sp ++;
            break;
        case 0x34:
            // INC (HL)  (INR M)
            cpu->motherboard_memory[GET_HL]++;
            set_zero_sign_parity_from_byte(cpu, cpu->motherboard_memory[GET_HL]);
            cpu->auxiliary_carry_flag = (bool)(((cpu->motherboard_memory[GET_HL]) & 0x0F) == 0);
            break;
        case 0x35:
            // DEC (HL)  (DCR M)
            cpu->motherboard_memory[GET_HL]--;
            set_zero_sign_parity_from_byte(cpu, cpu->motherboard_memory[GET_HL]);
            cpu->auxiliary_carry_flag = (bool)(((cpu->motherboard_memory[GET_HL]) & 0x0F) != 0x0F);
            break;
        case 0x36: 
            // MVI M, data (a.k.a. LD (HL), data)
            cpu->motherboard_memory[GET_HL] = cpu->motherboard_memory[cpu->pc + 1];
            pc_increments = 2;
            break;
        case 0x37: 
            // STC (set carry)
            cpu->carry_flag = true;
            break;
        case 0x39:
            // ADD HL, SP (DAD rp)
            // Only the carry flag is affected, and then if the 16-bit addition carries out.
            tmp_32 = (uint32_t)(GET_HL) + (uint32_t)(cpu->sp);
            cpu->carry_flag = (bool)(tmp_32 > 0xFFFF);
            cpu->h = (uint8_t)((tmp_32 & 0xFF00) >> 8);
            cpu->l = (uint8_t)(tmp_32 & 0x00FF);
            break;
        case 0x3A: 
            // LDA data 16 (load accumulator direct)
            cpu->a = cpu->motherboard_memory[DATA_TO_INT16];
            pc_increments = 3;
            break;
        case 0x3B: 
            // DEC SP (DCX rp)
            cpu->sp --;
            break;
        case 0x3C: 
            // INC A (INR r)
            cpu->a ++;
            set_zero_sign_parity_from_byte(cpu, cpu->a);
            cpu->auxiliary_carry_flag = (bool)(((cpu->a) & 0x0F) == 0);
            break;
        case 0x3D: 
            // DEC A (DCR r)
            cpu->a --;
            set_zero_sign_parity_from_byte(cpu, cpu->a);
            cpu->auxiliary_carry_flag = (bool)(((cpu->a) & 0x0F) != 0x0F);
            break;
        case 0x3E:
            // MVI A, d8  (LD A, d8)
            cpu->a = cpu->motherboard_memory[cpu->pc + 1];
            pc_increments = 2;
            break;
        case 0x3F: 
            // CMC (complement carry flag)
            cpu->carry_flag = !(cpu->carry_flag);
            break;
        case 0x40: 
            // LD B, B (MOV r1, r2)
            cpu->b = cpu->b;
            break;
        case 0x41: 
            // LD B, C (MOV r1, r2)
            cpu->b = cpu->c;
            break;
        case 0x42:
            // LD B, D (MOV r1, r2)
            cpu->b = cpu->d;
            break;
        case 0x43:
            // LD B, E (MOV r1, r2)
            cpu->b = cpu->e;
            break;
        case 0x44:
            // LD B, H (MOV r1, r2)
            cpu->b = cpu->h;
            break;
        case 0x45:
            // LD B, L (MOV r1, r2)
            cpu->b = cpu->l;
            break;
        case 0x46:
            // LD B, (HL) (MOV r, M)
            cpu->b = cpu->motherboard_memory[GET_HL];
            break;
        case 0x47: 
            // LD B, A (MOV r1, r2)
            cpu->b = cpu->a;
            break;
        case 0x48: 
            // LD C, B (MOV r1, r2)
            cpu->c= cpu->b;
            break;
        case 0x49: 
            // LD C, C (MOV r1, r2)
            cpu->c= cpu->c;
            break;
        case 0x4A: 
            // LD C, D (MOV r1, r2)
            cpu->c= cpu->d;
            break;
        case 0x4B: 
            // LD C, E (MOV r1, r2)
            cpu->c= cpu->e;
            break;
        case 0x4C: 
            // LD C, H (MOV r1, r2)
            cpu->c= cpu->h;
            break;
        case 0x4D:
            // LD C, L (MOV r1, r2)
            cpu->c= cpu->l;
            break;
        case 0x4E: 
            // LD C, (HL) (MOV r, M)
            cpu->c= cpu->motherboard_memory[GET_HL];
            break;
        case 0x4F: 
            // LD C, A (MOV r1, r2)
            cpu->c= cpu->a;
            break;
        case 0x50: 
            // LD D, B (MOV r1, r2)
            cpu->d= cpu->b;
            break;
        case 0x51: 
            // LD D, C (MOV r1, r2)
            cpu->d= cpu->c;
            break;
        case 0x52: 
            // LD D, D (MOV r1, r2)
            cpu->d= cpu->d;
            break;
        case 0x53: 
            // LD D, E (MOV r1, r2)
            cpu->d= cpu->e;
            break;
        case 0x54: 
            // LD D, H (MOV r1, r2)
            cpu->d= cpu->h;
            break;
        case 0x55: 
            // LD D, L (MOV r1, r2)
            cpu->d= cpu->l;
            break;
        case 0x56: 
            // LD D, (HL) (MOV r, M)
            cpu->d= cpu->motherboard_memory[GET_HL];
            break;
        case 0x57: 
            // LD D, A (MOV r1, r2)
            cpu->d= cpu->a;
            break;
        case 0x58: 
            // LD E, B (MOV r1, r2)
            cpu->e= cpu->b;
            break;
        case 0x59: 
            // LD E, C (MOV r1, r2)
            cpu->e= cpu->c;
            break;
        case 0x5A: 
            // LD E, D (MOV r1, r2)
            cpu->e= cpu->d;
            break;
        case 0x5B: 
           // LD E, E (MOV r1, r2)
            cpu->e= cpu->e;
            break;
        case 0x5C: 
            // LD E, H (MOV r1, r2)
            cpu->e= cpu->h;
            break;
        case 0x5D: 
            // LD E, L (MOV r1, r2)
            cpu->e= cpu->l;
            break;
        case 0x5E:
            // LD E, (HL) (MOV r, M)
            cpu->e= cpu->motherboard_memory[GET_HL];
            break;
        case 0x5F:
            // LD E, A (MOV r1, r2)
            cpu->e= cpu->a;
            break;
        case 0x60:
            // LD H, B (MOV r1, r2)
            cpu->h= cpu->b;
            break;
        case 0x61:
            // LD H, C (MOV r1, r2)
            cpu->h= cpu->c;
            break;
        case 0x62:
            // LD H, D (MOV r1, r2)
            cpu->h= cpu->d;
            break;
        case 0x63:
            // LD H, E (MOV r1, r2)
            cpu->h= cpu->e;
            break;
        case 0x64:
            // LD H, H (MOV r1, r2)
            cpu->h= cpu->h;
            break;
        case 0x65:
            // LD H, L (MOV r1, r2)
            cpu->h= cpu->l;
            break;
        case 0x66:
            // LD H, (HL) (MOV r, M)
            cpu->h= cpu->motherboard_memory[GET_HL];
            break;
        case 0x67:
            // LD H, A (MOV r1, r2)
            cpu->h= cpu->a;
            break;
        case 0x68: 
            // LD L, B (MOV r1, r2)
            cpu->l= cpu->b;
            break;
        case 0x69: 
            // LD L, C (MOV r1, r2)
            cpu->l= cpu->c;
            break;
        case 0x6A: 
            // LD L, D (MOV r1, r2)
            cpu->l= cpu->d;
            break;
        case 0x6B: 
            // LD L, E (MOV r1, r2)
            cpu->l= cpu->e;
            break;
        case 0x6C: 
            // LD L, H (MOV r1, r2)
            cpu->l= cpu->h;
            break;
        case 0x6D: 
            // LD L, L (MOV r1, r2)
            cpu->l= cpu->l;
            break;
        case 0x6E: 
            // LD L, (HL) (MOV r, M)
            cpu->l= cpu->motherboard_memory[GET_HL];
            break;
        case 0x6F: 
            // LD L, A (MOV r1, r2)
            cpu->l= cpu->a;
            break;
        case 0x70: 
            // LD (HL), B  (MOV M, r)
            cpu->motherboard_memory[GET_HL] = cpu->b;
            break;
        case 0x71: 
            // LD (HL), C  (MOV M, r)
            cpu->motherboard_memory[GET_HL] = cpu->c;
            break;
        case 0x72: 
            // LD (HL), D  (MOV M, r)
            cpu->motherboard_memory[GET_HL] = cpu->d;
            break;
        case 0x73:
            // LD (HL), E  (MOV M, r)
            cpu->motherboard_memory[GET_HL] = cpu->e;
            break;
        case 0x74: 
            // LD (HL), H  (MOV M, r)
            cpu->motherboard_memory[GET_HL] = cpu->h;
            break;
        case 0x75: 
            // LD (HL), L  (MOV M, r)
            cpu->motherboard_memory[GET_HL] = cpu->l;
            break;
        case 0x76: 
            // HLT
            cpu->halted = true;
            break;
        case 0x77: 
            // LD (HL), A  (MOV M, r)
            cpu->motherboard_memory[GET_HL] = cpu->a;
            break;
        case 0x78: 
            // LD A, B (MOV r1, r2)
            cpu->a= cpu->b;
            break;
        case 0x79: // MOV
            // LD A, C (MOV r1, r2)
            cpu->a= cpu->c;
            break;
        case 0x7A: // MOV
            // LD A, D (MOV r1, r2)
            cpu->a= cpu->d;
            break;
        case 0x7B: // MOV
            // LD A, E (MOV r1, r2)
            cpu->a= cpu->e;
            break;
        case 0x7C: // MOV
            // LD A, H (MOV r1, r2)
            cpu->a= cpu->h;
            break;
        case 0x7D: // MOV
            // LD A, L (MOV r1, r2)
            cpu->a= cpu->l;
            break;
        case 0x7E: // MOV
            // LD A, (HL) (MOV r, M)
            cpu->a= cpu->motherboard_memory[GET_HL];
            break;
        case 0x7F: // MOV
            // LD A, A (MOV r1, r2)
            cpu->a= cpu->a;
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