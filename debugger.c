#include <stdio.h>
#include "debugger.h"


void debug_dump_8080(cpu8080 cpu) {
    printf("PC: 0x%02X\n", cpu.pc);
    printf("A: 0x%02X\n", cpu.a);
    printf("Flags: ");
    if (cpu.zero_flag) {
        printf("Z, ");
    }
    else {
        printf("NZ, ");
    }
    if (cpu.carry_flag) {
        printf("C, ");
    }
    else {
        printf("NC, ");
    }
    if (cpu.sign_flag) {
        printf("M, ");
    }
    else {
        printf("P, ");
    }
    if (cpu.parity_flag) {
        printf("PE, ");
    }
    else {
        printf("PO, ");
    }
    if (cpu.auxiliary_carry_flag) {
        printf("Aux C\n");
    }
    else {
        printf("Aux NC\n");
    }
    printf("SP: 0x%02X\n", cpu.sp);
    printf("B: 0x%02X\tC: 0x%02X\n", cpu.b, cpu.c);
    printf("D: 0x%02X\tE: 0x%02X\n", cpu.d, cpu.e);
    printf("H: 0x%02X\tL: 0x%02X\n", cpu.h, cpu.l);
}