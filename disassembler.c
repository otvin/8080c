#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

void handle_error(){
    perror("Fatal error");
    exit(EXIT_FAILURE);
}


int parse_opcode(int addr, uint8_t *memory, char *strbuff) {
    int retval = 1;

    switch(memory[addr]) {
        case 0x00: // NOP
            sprintf(strbuff, "NOP");
            break;
        case 0x01: // LXI
            sprintf(strbuff, "LD BC, $%02X%02X\t;LXI rp, data 16", memory[addr+2], memory[addr+1]);
            retval=3;
            break;
        case 0x02: // STAX
            sprintf(strbuff, "STAX (BC)");
            break;
        case 0x03: // INX
            sprintf(strbuff, "INC BC\t; INX rp");
            break;
        case 0x04: // INR
            sprintf(strbuff, "INC B\t; INR r");
            break;
        case 0x05: // DCR
            sprintf(strbuff, "DEC B\t; DCR r");
            break;
        case 0x06: // MVI
            sprintf(strbuff, "LD B, $%02X\t;MVI r, data", memory[addr+1]);
            retval = 2;
            break;
        case 0x07: // RLC
            sprintf(strbuff, "RLC");
            break;
        case 0x09: // DAD
            sprintf(strbuff, "ADD HL, BC\t; DAD rp");
            break;
        case 0x0A: // LDAX
            sprintf(strbuff, "LDAX (BC)");
            break;
        case 0x0B: // DCX
            sprintf(strbuff, "DEC BC\t; DCX rp");
            break;
        case 0x0C: // INR
            sprintf(strbuff, "INC C\t; INR r");
            break;
        case 0x0D: // DCR
            sprintf(strbuff, "DEC C\t; DCR r");
            break;
        case 0x0E: // MVI
            sprintf(strbuff, "LD C, $%02X\t;MVI r, data", memory[addr+1]);
            retval = 2;
            break;
        case 0x0F: // RRC
            sprintf(strbuff, "RRCA");
            break;
        case 0x11: // LXI
            sprintf(strbuff, "LD DE, $%02X%02X\t;LXI rp, data 16", memory[addr+2], memory[addr+1]);
            retval=3;
            break;
        case 0x12: // STAX
            sprintf(strbuff, "STAX (DE)");
            break;
        case 0x13: // INX
            sprintf(strbuff, "INC DE\t; INX rp");
            break;
        case 0x14: // INR
            sprintf(strbuff, "INC D\t; INR r");
            break;
        case 0x15: // DCR
            sprintf(strbuff, "DEC D\t; DCR r");
            break;
        case 0x16: // MVI
            sprintf(strbuff, "LD D, $%02X\t;MVI r, data", memory[addr+1]);
            retval = 2;
            break;
        case 0x17: // RAL
            sprintf(strbuff, "RAL");
            break;
        case 0x19: // DAD
            sprintf(strbuff, "ADD HL, DE\t; DAD rp");
            break;
        case 0x1A: // LDAX
            sprintf(strbuff, "LDAX (DE)");
            break;
        case 0x1B: // DCX
            sprintf(strbuff, "DEC DE\t; DCX rp");
            break;
        case 0x1C: // INR
            sprintf(strbuff, "INC E\t; INR r");
            break;
        case 0x1D: // DCR
            sprintf(strbuff, "DEC E\t; DCR r");
            break;
        case 0x1E: // MVI
            sprintf(strbuff, "LD E, $%02X\t;MVI r, data", memory[addr+1]);
            retval = 2;
            break;
        case 0x1F: // RAR
            sprintf(strbuff, "RAR");
            break;
        case 0x21: // LXI
            sprintf(strbuff, "LD HL, $%02X%02X\t;LXI rp, data 16", memory[addr+2], memory[addr+1]);
            retval = 3;
            break;
        case 0x22: // SHLD
            sprintf(strbuff, "LD ($%02X%02X), HL\t; SHLD", memory[addr+2], memory[addr+1]);
            retval = 3;
            break;
        case 0x23: // INX
            sprintf(strbuff, "INC HL\t; INX rp");
            break;
        case 0x24: // INR
            sprintf(strbuff, "INC H\t; INR r");
            break;
        case 0x25: // DCR
            sprintf(strbuff, "DEC H\t; DCR r");
            break;
        case 0x26: // MVI
            sprintf(strbuff, "LD H, $%02X\t;MVI r, data", memory[addr+1]);
            retval = 2;
            break;
        case 0x27: // DAA
            sprintf(strbuff, "DAA");
            break;
        case 0x29: // DAD
            sprintf(strbuff, "ADD HL, HL\t; DAD rp");
            break;
        case 0x2A: // LHLD
            sprintf(strbuff, "LD HL, ($%02X%02X)\t; LHLD", memory[addr+2], memory[addr+1]);
            retval = 3;
            break;
        case 0x2B: // DCX
            sprintf(strbuff, "DEC HL\t; DCX rp");
            break;
        case 0x2C: // INR
            sprintf(strbuff, "INC L\t; INR r");
            break;
        case 0x2D: // DCR
            sprintf(strbuff, "DEC L\t; DCR r");
            break;
        case 0x2E: // MVI
            sprintf(strbuff, "LD L, $%02X\t;MVI r, data", memory[addr+1]);
            retval = 2;
            break;
        case 0x2F: // CMA
            sprintf(strbuff, "CMA");
            break;
        case 0x31: // LXI
            sprintf(strbuff, "LD SP, $%02X%02X\t;LXI rp, data 16", memory[addr+2], memory[addr+1]);
            retval = 3;
            break;
        case 0x32: // STA
            sprintf(strbuff, "LD ($%02X%02X), A\t; STA", memory[addr+2], memory[addr+1]);
            retval = 3;
            break;
        case 0x33: // INX
            sprintf(strbuff, "INC SP\t; INX rp");
            break;
        case 0x34: // INR
            sprintf(strbuff, "INC (HL)\t; INR M");
            break;
        case 0x35: // DCR
            sprintf(strbuff, "DEC (HL)\t; DCR M");
            break;
        case 0x36: // MVI
            sprintf(strbuff, "LD (HL), $%02X\t;MVI M, data", memory[addr+1]);
            retval = 2;
            break;
        case 0x37: // STC
            sprintf(strbuff, "STC");
            break;
        case 0x39: // DAD
            sprintf(strbuff, "ADD HL, SP\t; DAD rp");
            break;
        case 0x3A: // LDA
            sprintf(strbuff, "LD A, ($%02X%02X)\t; LDA", memory[addr+2], memory[addr+1]);
            retval = 3;
            break;
        case 0x3B: // DCX
            sprintf(strbuff, "DEC SP\t; DCX rp");
            break;
        case 0x3C: // INR
            sprintf(strbuff, "INC A\t; INR r");
            break;
        case 0x3D: // DCR
            sprintf(strbuff, "DEC A\t; DCR r");
            break;
        case 0x3E: // MVI
            sprintf(strbuff, "LD A, $%02X\t;MVI r, data", memory[addr+1]);
            retval = 2;
            break;
        case 0x3F: // CMC
            sprintf(strbuff, "CMC");
            break;
        case 0x40: // MOV
            sprintf(strbuff, "LD B, B\t; MOV r1, r2");
            break;
        case 0x41: // MOV
            sprintf(strbuff, "LD B, C\t; MOV r1, r2");
            break;
        case 0x42: // MOV
            sprintf(strbuff, "LD B, D\t; MOV r1, r2");
            break;
        case 0x43: // MOV
            sprintf(strbuff, "LD B, E\t; MOV r1, r2");
            break;
        case 0x44: // MOV
            sprintf(strbuff, "LD B, H\t; MOV r1, r2");
            break;
        case 0x45: // MOV
            sprintf(strbuff, "LD B, L\t; MOV r1, r2");
            break;
        case 0x46: // MOV
            sprintf(strbuff, "LD B, (HL)\t; MOV r, M");
            break;
        case 0x47: // MOV
            sprintf(strbuff, "LD B, A\t; MOV r1, r2");
            break;
        case 0x48: // MOV
            sprintf(strbuff, "LD C, B\t; MOV r1, r2");
            break;
        case 0x49: // MOV
            sprintf(strbuff, "LD C, C\t; MOV r1, r2");
            break;
        case 0x4A: // MOV
            sprintf(strbuff, "LD C, D\t; MOV r1, r2");
            break;
        case 0x4B: // MOV
            sprintf(strbuff, "LD C, E\t; MOV r1, r2");
            break;
        case 0x4C: // MOV
            sprintf(strbuff, "LD C, H\t; MOV r1, r2");
            break;
        case 0x4D: // MOV
            sprintf(strbuff, "LD C, L\t; MOV r1, r2");
            break;
        case 0x4E: // MOV
            sprintf(strbuff, "LD C, (HL)\t; MOV r, M");
            break;
        case 0x4F: // MOV
            sprintf(strbuff, "LD C, A\t; MOV r1, r2");
            break;
        case 0x50: // MOV
            sprintf(strbuff, "LD D, B\t; MOV r1, r2");
            break;
        case 0x51: // MOV
            sprintf(strbuff, "LD D, C\t; MOV r1, r2");
            break;
        case 0x52: // MOV
            sprintf(strbuff, "LD D, D\t; MOV r1, r2");
            break;
        case 0x53: // MOV
            sprintf(strbuff, "LD D, E\t; MOV r1, r2");
            break;
        case 0x54: // MOV
            sprintf(strbuff, "LD D, H\t; MOV r1, r2");
            break;
        case 0x55: // MOV
            sprintf(strbuff, "LD D, L\t; MOV r1, r2");
            break;
        case 0x56: // MOV
            sprintf(strbuff, "LD D, (HL)\t; MOV r, M");
            break;
        case 0x57: // MOV
            sprintf(strbuff, "LD D, A\t; MOV r1, r2");
            break;
        case 0x58: // MOV
            sprintf(strbuff, "LD E, B\t; MOV r1, r2");
            break;
        case 0x59: // MOV
            sprintf(strbuff, "LD E, C\t; MOV r1, r2");
            break;
        case 0x5A: // MOV
            sprintf(strbuff, "LD E, D\t; MOV r1, r2");
            break;
        case 0x5B: // MOV
            sprintf(strbuff, "LD E, E\t; MOV r1, r2");
            break;
        case 0x5C: // MOV
            sprintf(strbuff, "LD E, H\t; MOV r1, r2");
            break;
        case 0x5D: // MOV
            sprintf(strbuff, "LD E, L\t; MOV r1, r2");
            break;
        case 0x5E: // MOV
            sprintf(strbuff, "LD E, (HL)\t; MOV r, M");
            break;
        case 0x5F: // MOV
            sprintf(strbuff, "LD E, A\t; MOV r1, r2");
            break;
        case 0x60: // MOV
            sprintf(strbuff, "LD H, B\t; MOV r1, r2");
            break;
        case 0x61: // MOV
            sprintf(strbuff, "LD H, C\t; MOV r1, r2");
            break;
        case 0x62: // MOV
            sprintf(strbuff, "LD H, D\t; MOV r1, r2");
            break;
        case 0x63: // MOV
            sprintf(strbuff, "LD H, E\t; MOV r1, r2");
            break;
        case 0x64: // MOV
            sprintf(strbuff, "LD H, H\t; MOV r1, r2");
            break;
        case 0x65: // MOV
            sprintf(strbuff, "LD H, L\t; MOV r1, r2");
            break;
        case 0x66: // MOV
            sprintf(strbuff, "LD H, (HL)\t; MOV r, M");
            break;
        case 0x67: // MOV
            sprintf(strbuff, "LD H, A\t; MOV r1, r2");
            break;
        case 0x68: // MOV
            sprintf(strbuff, "LD L, B\t; MOV r1, r2");
            break;
        case 0x69: // MOV
            sprintf(strbuff, "LD L, C\t; MOV r1, r2");
            break;
        case 0x6A: // MOV
            sprintf(strbuff, "LD L, D\t; MOV r1, r2");
            break;
        case 0x6B: // MOV
            sprintf(strbuff, "LD L, E\t; MOV r1, r2");
            break;
        case 0x6C: // MOV
            sprintf(strbuff, "LD L, H\t; MOV r1, r2");
            break;
        case 0x6D: // MOV
            sprintf(strbuff, "LD L, L\t; MOV r1, r2");
            break;
        case 0x6E: // MOV
            sprintf(strbuff, "LD L, (HL)\t; MOV r, M");
            break;
        case 0x6F: // MOV
            sprintf(strbuff, "LD L, A\t; MOV r1, r2");
            break;
        case 0x70: // MOV
            sprintf(strbuff, "LD (HL), B\t; MOV M, r");
            break;
        case 0x71: // MOV
            sprintf(strbuff, "LD (HL), C\t; MOV M, r");
            break;
        case 0x72: // MOV
            sprintf(strbuff, "LD (HL), D\t; MOV M, r");
            break;
        case 0x73: // MOV
            sprintf(strbuff, "LD (HL), E\t; MOV M, r");
            break;
        case 0x74: // MOV
            sprintf(strbuff, "LD (HL), H\t; MOV M, r");
            break;
        case 0x75: // MOV
            sprintf(strbuff, "LD (HL), L\t; MOV M, r");
            break;
        case 0x76: // HLT
            sprintf(strbuff, "HLT");
            break;
        case 0x77: // MOV
            sprintf(strbuff, "LD (HL), A\t; MOV M, r");
            break;
        case 0x78: // MOV
            sprintf(strbuff, "LD A, B\t; MOV r1, r2");
            break;
        case 0x79: // MOV
            sprintf(strbuff, "LD A, C\t; MOV r1, r2");
            break;
        case 0x7A: // MOV
            sprintf(strbuff, "LD A, D\t; MOV r1, r2");
            break;
        case 0x7B: // MOV
            sprintf(strbuff, "LD A, E\t; MOV r1, r2");
            break;
        case 0x7C: // MOV
            sprintf(strbuff, "LD A, H\t; MOV r1, r2");
            break;
        case 0x7D: // MOV
            sprintf(strbuff, "LD A, L\t; MOV r1, r2");
            break;
        case 0x7E: // MOV
            sprintf(strbuff, "LD A, (HL)\t; MOV r, M");
            break;
        case 0x7F: // MOV
            sprintf(strbuff, "LD A, A\t; MOV r1, r2");
            break;
        case 0x80: // ADD
            sprintf(strbuff, "ADD A, B");
            break;
        case 0x81: // ADD
            sprintf(strbuff, "ADD A, C");
            break;
        case 0x82: // ADD
            sprintf(strbuff, "ADD A, D");
            break;
        case 0x83: // ADD
            sprintf(strbuff, "ADD A, E");
            break;
        case 0x84: // ADD
            sprintf(strbuff, "ADD A, H");
            break;
        case 0x85: // ADD
            sprintf(strbuff, "ADD A, L");
            break;
        case 0x86: // ADD
            sprintf(strbuff, "ADD A, (HL)");
            break;
        case 0x87: // ADD
            sprintf(strbuff, "ADD A, A");
            break;
        case 0x88: // ADC
            sprintf(strbuff, "ADC A, B");
            break;
        case 0x89: // ADC
            sprintf(strbuff, "ADC A, C");
            break;
        case 0x8A: // ADC
            sprintf(strbuff, "ADC A, D");
            break;
        case 0x8B: // ADC
            sprintf(strbuff, "ADC A, E");
            break;
        case 0x8C: // ADC
            sprintf(strbuff, "ADC A, H");
            break;
        case 0x8D: // ADC
            sprintf(strbuff, "ADC A, L");
            break;
        case 0x8E: // ADC
            sprintf(strbuff, "ADC A, (HL)");
            break;
        case 0x8F: // ADC
            sprintf(strbuff, "ADC A, A");
            break;
        case 0x90: // SUB
            sprintf(strbuff, "SUB A, B");
            break;
        case 0x91: // SUB
            sprintf(strbuff, "SUB A, C");
            break;
        case 0x92: // SUB
            sprintf(strbuff, "SUB A, D");
            break;
        case 0x93: // SUB
            sprintf(strbuff, "SUB A, E");
            break;
        case 0x94: // SUB
            sprintf(strbuff, "SUB A, H");
            break;
        case 0x95: // SUB
            sprintf(strbuff, "SUB A, L");
            break;
        case 0x96: // SUB
            sprintf(strbuff, "SUB A, (HL)");
            break;
        case 0x97: // SUB
            sprintf(strbuff, "SUB A, A");
            break;
        case 0x98: // SBB
            sprintf(strbuff, "SBB A, B");
            break;
        case 0x99: // SBB
            sprintf(strbuff, "SBB A, C");
            break;
        case 0x9A: // SBB
            sprintf(strbuff, "SBB A, D");
            break;
        case 0x9B: // SBB
            sprintf(strbuff, "SBB A, E");
            break;
        case 0x9C: // SBB
            sprintf(strbuff, "SBB A, H");
            break;
        case 0x9D: // SBB
            sprintf(strbuff, "SBB A, L");
            break;
        case 0x9E: // SBB
            sprintf(strbuff, "SBB A, (HL)");
            break;
        case 0x9F: // SBB
            sprintf(strbuff, "SBB A, A");
            break;
        case 0xA0: // ANA
            sprintf(strbuff, "AND B\t; ANA r");
            break;
        case 0xA1: // ANA
            sprintf(strbuff, "AND C\t; ANA r");
            break;
        case 0xA2: // ANA
            sprintf(strbuff, "AND D\t; ANA r");
            break;
        case 0xA3: // ANA
            sprintf(strbuff, "AND E\t; ANA r");
            break;
        case 0xA4: // ANA
            sprintf(strbuff, "AND H\t; ANA r");
            break;
        case 0xA5: // ANA
            sprintf(strbuff, "AND L\t; ANA r");
            break;
        case 0xA6: // ANA
            sprintf(strbuff, "AND (HL)\t; ANA M");
            break;
        case 0xA7: // ANA
            sprintf(strbuff, "AND A\t; ANA r");
            break;
        case 0xA8: // XRA
            sprintf(strbuff, "XOR B\t; XRA r");
            break;
        case 0xA9: // XRA
            sprintf(strbuff, "XOR C\t; XRA r");
            break;
        case 0xAA: // XRA
            sprintf(strbuff, "XOR D\t; XRA r");
            break;
        case 0xAB: // XRA
            sprintf(strbuff, "XOR E\t; XRA r");
            break;
        case 0xAC: // XRA
            sprintf(strbuff, "XOR H\t; XRA r");
            break;
        case 0xAD: // XRA
            sprintf(strbuff, "XOR L\t; XRA r");
            break;
        case 0xAE: // XRA
            sprintf(strbuff, "XOR (HL)\t; XRA M");
            break;
        case 0xAF: // XRA
            sprintf(strbuff, "XOR A\t; XRA r");
            break;
        case 0xB0: // ORA
            sprintf(strbuff, "OR B\t; ORA r");
            break;
        case 0xB1: // ORA
            sprintf(strbuff, "OR C\t; ORA r");
            break;
        case 0xB2: // ORA
            sprintf(strbuff, "OR D\t; ORA r");
            break;
        case 0xB3: // ORA
            sprintf(strbuff, "OR E\t; ORA r");
            break;
        case 0xB4: // ORA
            sprintf(strbuff, "OR H\t; ORA r");
            break;
        case 0xB5: // ORA
            sprintf(strbuff, "OR L\t; ORA r");
            break;
        case 0xB6: // ORA
            sprintf(strbuff, "OR (HL)\t; ORA M");
            break;
        case 0xB7: // ORA
            sprintf(strbuff, "OR A\t; ORA r");
            break;
        case 0xB8: // CMP
            sprintf(strbuff, "CMP B");
            break;
        case 0xB9: // CMP
            sprintf(strbuff, "CMP C");
            break;
        case 0xBA: // CMP
            sprintf(strbuff, "CMP D");
            break;
        case 0xBB: // CMP
            sprintf(strbuff, "CMP E");
            break;
        case 0xBC: // CMP
            sprintf(strbuff, "CMP H");
            break;
        case 0xBD: // CMP
            sprintf(strbuff, "CMP L");
            break;
        case 0xBE: // CMP
            sprintf(strbuff, "CMP (HL)");
            break;
        case 0xBF: // CMP
            sprintf(strbuff, "CMP A");
            break;
        case 0xC0: // RET
            sprintf(strbuff, "RET NZ");
            break;
        case 0xC1: // POP
            sprintf(strbuff, "POP BC");
            break;
        case 0xC2: // JMP
            sprintf(strbuff, "JMP NZ $%02X%02X", memory[addr+2], memory[addr+1]);
            retval = 3;
            break;
        case 0xC3: // JMP
            sprintf(strbuff, "JMP $%02X%02X", memory[addr+2], memory[addr+1]);
            retval = 3;
            break;
        case 0xC4: // CALL
            sprintf(strbuff, "CALL NZ $%02X%02X", memory[addr+2], memory[addr+1]);
            retval = 3;
            break;
        case 0xC5: // PUSH
            sprintf(strbuff, "PUSH BC");
            break;
        case 0xC6: // ADI
            sprintf(strbuff, "ADD A, $%02X\t; ADI data", memory[addr+1]);
            retval = 2;
            break;
        case 0xC7: // RST
            sprintf(strbuff, "RST 000");
            break;
        case 0xC8: // RET
            sprintf(strbuff, "RET Z");
            break;
        case 0xC9: // RET
            sprintf(strbuff, "RET");
            break;
        case 0xCA: // JMP
            sprintf(strbuff, "JMP Z $%02X%02X", memory[addr+2], memory[addr+1]);
            retval = 3;
            break;
        case 0xCC: // CALL
            sprintf(strbuff, "CALL Z $%02X%02X", memory[addr+2], memory[addr+1]);
            retval = 3;
            break;
        case 0xCD: // CALL
            sprintf(strbuff, "CALL $%02X%02X", memory[addr+2], memory[addr+1]);
            retval = 3;
            break;
        case 0xCE: // ACI
            sprintf(strbuff, "ACI A, $%02X", memory[addr+1]);
            retval = 2;
            break;
        case 0xCF: // RST
            sprintf(strbuff, "RST 001");
            break;
        case 0xD0: // RET
            sprintf(strbuff, "RET NC");
            break;
        case 0xD1: // POP
            sprintf(strbuff, "POP DE");
            break;
        case 0xD2: // JMP
            sprintf(strbuff, "JMP NC $%02X%02X", memory[addr+2], memory[addr+1]);
            retval = 3;
            break;
        case 0xD3: // OUT
            sprintf(strbuff, "OUT (OUT $%02X), A", memory[addr+1]);
            retval = 2;
            break;
        case 0xD4: // CALL
            sprintf(strbuff, "CALL NC $%02X%02X", memory[addr+2], memory[addr+1]);
            retval = 3;
            break;
        case 0xD5: // PUSH
            sprintf(strbuff, "PUSH DE");
            break;
        case 0xD6: // SUI
            sprintf(strbuff, "SUB A, $%02X\t; SUI data", memory[addr+1]);
            retval = 2;
            break;
        case 0xD7: // RST
            sprintf(strbuff, "RST 002");
            break;
        case 0xD8: // RET
            sprintf(strbuff, "RET C");
            break;
        case 0xDA: // JMP
            sprintf(strbuff, "JMP C $%02X%02X", memory[addr+2], memory[addr+1]);
            retval = 3;
            break;
        case 0xDB: // IN
            sprintf(strbuff, "IN A, (INP %02X)", memory[addr+1]);
            retval = 2;
            break;
        case 0xDC: // CALL
            sprintf(strbuff, "CALL C $%02X%02X", memory[addr+2], memory[addr+1]);
            retval = 3;
            break;
        case 0xDE: // SBI
            sprintf(strbuff, "SBI A, $%02X", memory[addr+1]);
            retval = 2;
            break;
        case 0xDF: // RST
            sprintf(strbuff, "RST 003");
            break;
        case 0xE0: // RET
            sprintf(strbuff, "RET PO");
            break;
        case 0xE1: // POP
            sprintf(strbuff, "POP HL");
            break;
        case 0xE2: // JMP
            sprintf(strbuff, "JMP PO $%02X%02X", memory[addr+2], memory[addr+1]);
            retval = 3;
            break;
        case 0xE3: // XTHL
            sprintf(strbuff, "XTHL");
            break;
        case 0xE4: // CALL
            sprintf(strbuff, "CALL PO $%02X%02X", memory[addr+2], memory[addr+1]);
            retval = 3;
            break;
        case 0xE5: // PUSH
            sprintf(strbuff, "PUSH HL");
            break;
        case 0xE6: // ANI
            sprintf(strbuff, "AND $%02X\t; ANI data", memory[addr+1]);
            retval = 2;
            break;
        case 0xE7: // RST
            sprintf(strbuff, "RST 004");
            break;
        case 0xE8: // RET
            sprintf(strbuff, "RET PE");
            break;
        case 0xE9: // PCHL
            sprintf(strbuff, "PCHL");
            break;
        case 0xEA: // JMP
            sprintf(strbuff, "JMP PE $%02X%02X", memory[addr+2], memory[addr+1]);
            retval = 3;
            break;
        case 0xEB: // XCHG
            sprintf(strbuff, "EX DE,HL\t; XCHG");
            break;
        case 0xEC: // CALL
            sprintf(strbuff, "CALL PE $%02X%02X", memory[addr+2], memory[addr+1]);
            retval = 3;
            break;
        case 0xEE: // XRI
            sprintf(strbuff, "XOR $%02X\t; XRI data", memory[addr+1]);
            retval = 2;
            break;
        case 0xEF: // RST
            sprintf(strbuff, "RST 005");
            break;
        case 0xF0: // RET
            sprintf(strbuff, "RET P");
            break;
        case 0xF1: // POP
            sprintf(strbuff, "POP AF");
            break;
        case 0xF2: // JMP
            sprintf(strbuff, "JMP P $%02X%02X", memory[addr+2], memory[addr+1]);
            retval = 3;
            break;
        case 0xF3: // DI
            sprintf(strbuff, "DI");
            break;
        case 0xF4: // CALL
            sprintf(strbuff, "CALL P $%02X%02X", memory[addr+2], memory[addr+1]);
            retval = 3;
            break;
        case 0xF5: // PUSH
            sprintf(strbuff, "PUSH AF");
            break;
        case 0xF6: // ORI
            sprintf(strbuff, "OR $%02X\t; ORI data", memory[addr+1]);
            retval = 2;
            break;
        case 0xF7: // RST
            sprintf(strbuff, "RST 006");
            break;
        case 0xF8: // RET
            sprintf(strbuff, "RET M");
            break;
        case 0xF9: // SPHL
            sprintf(strbuff, "SPHL");
            break;
        case 0xFA: // JMP
            sprintf(strbuff, "JMP M $%02X%02X", memory[addr+2], memory[addr+1]);
            retval = 3;
            break;
        case 0xFB: // EI
            sprintf(strbuff, "EI");
            break;
        case 0xFC: // CALL
            sprintf(strbuff, "CALL M $%02X%02X", memory[addr+2], memory[addr+1]);
            retval = 3;
            break;
        case 0xFE: // CPI
            sprintf(strbuff, "CMP $%02X\t; CPI data", memory[addr+1]);
            retval = 2;
            break;
        case 0xFF: // RST
            sprintf(strbuff, "RST 007");
            break;
        default:  // 0x08, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38, 0xCB, 0xD9, 0xDD, 0xED, 0xFD
            sprintf(strbuff, "??");
    }
    return retval;
}




void disassemble(int start_addr, int max_addr, int point_addr, uint8_t *memory){
    int cur_addr, res;
    char mnemonic[64];

    cur_addr = start_addr;
    while (cur_addr <= max_addr){
        if (cur_addr == point_addr) {
            printf("--->");
        }
        res = parse_opcode(cur_addr, memory, mnemonic);
        printf("\t%04X: ", cur_addr);
        // display the opcode
        printf("%02X ", memory[cur_addr]);
        if (res > 1) {
            printf("%02X ", memory[cur_addr+1]);
            if (res > 2) {
                printf("%02X ", memory[cur_addr+2]);
            }
            else {
                printf("   ");
            }
        }
        else {
            printf("      ");
        }
        printf("%s\n", mnemonic);
        cur_addr = cur_addr + res;
    }
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

void load_roms(uint8_t *memory) {
    int i;
    load_rom("invaders.h", 0x0000, 0x07FF, memory);
    load_rom("invaders.g", 0x0800, 0x0FFF, memory);
    load_rom("invaders.f", 0x1000, 0x17FF, memory);
    load_rom("invaders.e", 0x1800, 0x1FFF, memory);
}

void main() {
    uint8_t memory[0x10000];
    load_roms(memory);
    disassemble(0x0, 0x0BF4, -1, memory);
    disassemble(0x1000, 0x13FD, -1, memory);
    disassemble(0x1400, 0x19BB, -1, memory);
    disassemble(0x19D1, 0x1A10, -1, memory);
    disassemble(0x1A32, 0x1A90, -1, memory);
}

