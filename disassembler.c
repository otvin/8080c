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
            sprintf(strbuff, "LD BC, $%2X%2X\t;LXI rp, data 16", memory[addr+2], memory[addr+1]);
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
            sprintf(strbuff, "LD B, $%2X\t;MVI r, data", memory[addr+1]);
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
            sprintf(strbuff, "LD C, $%2X\t;MVI r, data", memory[addr+1]);
            retval = 2;
            break;
        case 0x0F: // RRC
            sprintf(strbuff, "RRCA");
            break;
        case 0x11: // LXI
            sprintf(strbuff, "LD DE, $%2X%2X\t;LXI rp, data 16", memory[addr+2], memory[addr+1]);
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
            sprintf(strbuff, "LD D, $%2X\t;MVI r, data", memory[addr+1]);
            retval = 2;
            break;
        case 0x17: // RAL
            srpintf(strbuff, "RAL");
            break;
        case 0x19: // DAD
            sprintf(strbuff, "ADD HL, BC\t; DAD rp");
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
            sprintf(strbuff, "LD E, $%2X\t;MVI r, data", memory[addr+1]);
            retval = 2;
            break;
        case 0x1F: // RAR
            sprintf(strbuff, "RAR");
            break;
        case 0x21: // LXI
            sprintf(strbuff, "LD HL, $%2X%2X\t;LXI rp, data 16", memory[addr+2], memory[addr+1]);
            retval = 3;
            break;
        case 0x22: // SHLD
            sprintf(strbuff, "LD ($%2X%2X), HL\t; SHLD", memory[addr+2], memory[addr+1]);
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
            sprintf(strbuff, "LD H, $%2X\t;MVI r, data", memory[addr+1]);
            retval = 2;
            break;
        case 0x27: // DAA
            sprintf(strbuff, "DAA");
            break;
        case 0x29: // DAD
            sprintf(strbuff, "ADD HL, HL\t; DAD rp");
            break;
        case 0x2A: // LHLD
            sprintf(strbuff, "LD HL, ($%2X%2X), HL\t; LHLD", memory[addr+2], memory[addr+1]);
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
            sprintf(strbuff, "LD L, $%2X\t;MVI r, data", memory[addr+1]);
            retval = 2;
            break;
        case 0x2F: // CMA
            sprintf(strbuff, "CMA");
            break;
        case 0x31: // LXI
            sprintf(strbuff, "LD SP, $%2X%2X\t;LXI rp, data 16", memory[addr+2], memory[addr+1]);
            retval = 3;
            break;
        case 0x32: // STA
            sprintf(strbuff, "LD ($%2X%2X), A\t; STA", memory[addr+2], memory[addr+1]);
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
            sprintf(strbuff, "LD (HL), $%2X\t;MVI M, data", memory[addr+1]);
            retval = 2;
            break;
        case 0x37: // STC
            sprintf(strbuff, "STC");
            break;
        case 0x39: // DAD
            sprintf(strbuff, "ADD HL, SP\t; DAD rp");
            break;
        case 0x3A: // LDA
            sprintf(strbuff, "LD A, ($%2X%2X)\t; LDA", memory[addr+2], memory[addr+1]);
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
            sprintf(strbuff, "LD A, $%2X\t;MVI r, data", memory[addr+1]);
            retval = 2;
            break;
        case 0x3F: // CMC
            sprintf(strbuff, "CMC");
            break;
        case 0x40: // MOV
        case 0x41: // MOV
        case 0x42: // MOV
        case 0x43: // MOV
        case 0x44: // MOV
        case 0x45: // MOV
        case 0x46: // MOV
        case 0x47: // MOV
        case 0x48: // MOV
        case 0x49: // MOV
        case 0x4A: // MOV
        case 0x4B: // MOV
        case 0x4C: // MOV
        case 0x4D: // MOV
        case 0x4E: // MOV
        case 0x4F: // MOV
        case 0x50: // MOV
        case 0x51: // MOV
        case 0x52: // MOV
        case 0x53: // MOV
        case 0x54: // MOV
        case 0x55: // MOV
        case 0x56: // MOV
        case 0x57: // MOV
        case 0x58: // MOV
        case 0x59: // MOV
        case 0x5A: // MOV
        case 0x5B: // MOV
        case 0x5C: // MOV
        case 0x5D: // MOV
        case 0x5E: // MOV
        case 0x5F: // MOV
        case 0x60: // MOV
        case 0x61: // MOV
        case 0x62: // MOV
        case 0x63: // MOV
        case 0x64: // MOV
        case 0x65: // MOV
        case 0x66: // MOV
        case 0x67: // MOV
        case 0x68: // MOV
        case 0x69: // MOV
        case 0x6A: // MOV
        case 0x6B: // MOV
        case 0x6C: // MOV
        case 0x6D: // MOV
        case 0x6E: // MOV
        case 0x6F: // MOV
        case 0x70: // MOV
        case 0x71: // MOV
        case 0x72: // MOV
        case 0x73: // MOV
        case 0x74: // MOV
        case 0x75: // MOV
        case 0x76: // HLT
        case 0x77: // MOV
        case 0x78: // MOV
        case 0x79: // MOV
        case 0x7A: // MOV
        case 0x7B: // MOV
        case 0x7C: // MOV
        case 0x7D: // MOV
        case 0x7E: // MOV
        case 0x7F: // MOV
        case 0x80: // ADD
        case 0x81: // ADD
        case 0x82: // ADD
        case 0x83: // ADD
        case 0x84: // ADD
        case 0x85: // ADD
        case 0x86: // ADD
        case 0x87: // ADD
        case 0x88: // ADC
        case 0x89: // ADC
        case 0x8A: // ADC
        case 0x8B: // ADC
        case 0x8C: // ADC
        case 0x8D: // ADC
        case 0x8E: // ADC
        case 0x8F: // ADC
        case 0x90: // SUB
        case 0x91: // SUB
        case 0x92: // SUB
        case 0x93: // SUB
        case 0x94: // SUB
        case 0x95: // SUB
        case 0x96: // SUB
        case 0x97: // SUB
        case 0x98: // SBB
        case 0x99: // SBB
        case 0x9A: // SBB
        case 0x9B: // SBB
        case 0x9C: // SBB
        case 0x9D: // SBB
        case 0x9E: // SBB
        case 0x9F: // SBB
        case 0xA0: // ANA
        case 0xA1: // ANA
        case 0xA2: // ANA
        case 0xA3: // ANA
        case 0xA4: // ANA
        case 0xA5: // ANA
        case 0xA6: // ANA
        case 0xA7: // ANA
        case 0xA8: // XRA
        case 0xA9: // XRA
        case 0xAA: // XRA
        case 0xAB: // XRA
        case 0xAC: // XRA
        case 0xAD: // XRA
        case 0xAE: // XRA
        case 0xAF: // XRA
        case 0xB0: // ORA
        case 0xB1: // ORA
        case 0xB2: // ORA
        case 0xB3: // ORA
        case 0xB4: // ORA
        case 0xB5: // ORA
        case 0xB6: // ORA
        case 0xB7: // ORA
        case 0xB8: // CMP
        case 0xB9: // CMP
        case 0xBA: // CMP
        case 0xBB: // CMP
        case 0xBC: // CMP
        case 0xBD: // CMP
        case 0xBE: // CMP
        case 0xBF: // CMP
        case 0xC0: // RET
        case 0xC1: // POP
        case 0xC2: // JMP
        case 0xC3: // JMP
        case 0xC4: // CALL
        case 0xC5: // PUSH
        case 0xC6: // ADI
        case 0xC7: // RST
        case 0xC8: // RET
        case 0xC9: // RET
        case 0xCA: // JMP
        case 0xCC: // CALL
        case 0xCD: // CALL
        case 0xCE: // ACI
        case 0xCF: // RST
        case 0xD0: // RET
        case 0xD1: // POP
        case 0xD2: // JMP
        case 0xD3: // OUT
        case 0xD4: // CALL
        case 0xD5: // PUSH
        case 0xD6: // SUI
        case 0xD7: // RST
        case 0xD8: // RET
        case 0xDA: // JMP
        case 0xDB: // IN
        case 0xDC: // CALL
        case 0xDE: // SBI
        case 0xDF: // RST
        case 0xE0: // RET
        case 0xE1: // POP
        case 0xE2: // JMP
        case 0xE3: // XTHL
        case 0xE4: // CALL
        case 0xE5: // PUSH
        case 0xE6: // ANI
        case 0xE7: // RST
        case 0xE8: // RET
        case 0xE9: // PCHL
        case 0xEA: // JMP
        case 0xEB: // XCHG
        case 0xEC: // CALL
        case 0xEE: // XRI
        case 0xEF: // RST
        case 0xF0: // RET
        case 0xF1: // POP
        case 0xF2: // JMP
        case 0xF3: // DI
        case 0xF4: // CALL
        case 0xF5: // PUSH
        case 0xF6: // ORI
        case 0xF7: // RST
        case 0xF8: // RET
        case 0xF9: // SPHL
        case 0xFA: // JMP
        case 0xFB: // EI
        case 0xFC: // CALL
        case 0xFE: // CPI
        case 0xFF: // RST
        
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
        res = parse_opcode(cur_addr, memory, mnemonic);
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
    disassemble(0x0000, 0x0001, 0, memory);
}

