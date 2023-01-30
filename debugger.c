#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include "debugger.h"
#include "disassembler.h"

#define RUN_FOREVER -1

void display_help() {
    printf("Debugger commands\n");
    printf("     ?           Display this list\n");
    printf("     s           Execute next line\n");
    printf("     <enter>     Execute next line\n");
    printf("     s N         Execute next N lines, but will stop at a breakpoint\n");
    printf("     b           Puts a breakpoint at the current line\n");
    printf("     b 0xM       Puts a breakpoint at address M\n");
    printf("     d 0xM       Deletes breakpoint at address M\n");
    printf("     info b      list breakpoints\n");
    printf("     bt          Prints the current stack\n");
    printf("     r           Run program until next breakpoint\n");
    printf("     rr          Exit debugger and return to normal execution\n");
    printf("     q           Terminate program and debugger\n");
    //printf("     int N       Send interrupt N to the system\n");
    printf("     x 0xM       display contents of memory address M\n");
    printf("     x 0xM N     display contents of N bytes of memory starting with address M\n");
    printf("     set 0xM 0xN set contents of memory address M with value N\n");
    //printf("     draw        tell video card to render the current screen\n");
}

void debug_dump_8080(cpu8080 cpu) {
    printf("PC: 0x%04X\n", cpu.pc);
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
    printf("SP: 0x%04X\n", cpu.sp);
    printf("B: 0x%02X\tC: 0x%02X\n", cpu.b, cpu.c);
    printf("D: 0x%02X\tE: 0x%02X\n", cpu.d, cpu.e);
    printf("H: 0x%02X\tL: 0x%02X\n", cpu.h, cpu.l);
}

bool set_breakpoint(int *breakpoint_list, uint16_t breakpoint, int list_size) {
    int i=0;
    bool added = false;
    while (i < list_size && !added) {
        if (breakpoint_list[i] == -1) {
            breakpoint_list[i] = breakpoint;
            added = true;
        }
        i++;
    }
    return(added);
}

bool clear_breakpoint(int *breakpoint_list, uint16_t breakpoint, int list_size) {
    int i=0;
    bool removed = false;
    while (i < list_size && !removed) {
        if (breakpoint_list[i] == breakpoint) {
            breakpoint_list[i] = -1;
            removed = true;
        }
        i++;
    }
    return (removed);
}

bool is_breakpoint(int *breakpoint_list, uint16_t addr, int list_size) {
    int i = 0;
    bool found = false;
    while (i < list_size && !found) {
        if (breakpoint_list[i] == addr) {
            found = true;
        }
        i++;
    }
    return(found);
}

void print_breakpoints(int *breakpoint_list, int list_size) {
    int i;
    bool foundone = false;
    printf("Breakpoint list\n");
    printf("---------------\n");
    for (i = 0; i < list_size; i++) {
        if (breakpoint_list[i] != -1) {
            printf("0x%04X\n", breakpoint_list[i]);
            foundone = true;
        }
    } 
    if (!foundone) {
        printf("No breakpoints set.\n");
    }
    printf("\n");
}

bool parse_long(const char *str, long *val) {
    // https://stackoverflow.com/questions/14176123/correct-usage-of-strtol
    char *temp;
    bool rc = true;
    errno = 0;
    *val = strtol(str, &temp, 0);
    if (temp == str || *temp != '\0' || ((*val == LONG_MIN || *val == LONG_MAX) && errno == ERANGE))
        rc = false;
    return rc;
}

// Returns true if execution should continue; false if execution should stop.
bool debug_8080(motherboard8080 motherboard, uint64_t *total_states, uint64_t *total_instructions) {
    int breakpoint_list[16] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
    const int breakpoint_list_size = 16;
    char cmd_buffer[32], parsed_command0[32], parsed_command1[32], parsed_command2[32];
    bool running = true, retval = true;
    int start, end, done, alldone, i, cmdpos, instr_to_run; 
    uint64_t num_states;
    long hex1, hex2;
    bool is_valid, keep_running, continue_on_return = true;
    

    while(running) {
        printf("\n");
        debug_dump_8080(motherboard.cpu);
        printf("\n");
        printf("Instructions: %ld\tStates: %ld\n\n", *total_instructions, *total_states);
        printf("Current +/- 10 bytes of instructions:\n");
        start = (motherboard.cpu.pc >= 0xA) ? (motherboard.cpu.pc) - 10 : 0;
        end = (motherboard.cpu.pc <= 0xFFF6) ? (motherboard.cpu.pc) + 10 : 0xFFFF;
        disassemble(start, end, motherboard.cpu.pc, breakpoint_list, breakpoint_list_size, motherboard.memory);
        printf("\n");

        printf("> ");
        parsed_command0[0] = (char)0;
        parsed_command1[0] = (char)0;
        parsed_command2[0] = (char)0;
        if (fgets(cmd_buffer, 32, stdin) != NULL) {
            // convert to lower case, split on spaces, and remove the carriage return that is included in fgets
            i = 0;
            cmdpos = 0;
            done = false; // done with each word
            alldone = false; // done parsing the command
            while(i < 32 && !done && !alldone) {
                if ((int) cmd_buffer[i] == 10) {
                    parsed_command0[cmdpos] = (char)0;
                    alldone = true;
                }
                else if ((int) cmd_buffer[i] == 32) {
                    parsed_command0[cmdpos] = (char)0;
                    i++;
                    done = true;
                }
                else {
                    parsed_command0[cmdpos] = tolower(cmd_buffer[i]);
                    i++;
                    cmdpos++;
                }
            }
            cmdpos = 0;
            done = false;
            while(i < 32 && !done && !alldone) {
                if ((int) cmd_buffer[i] == 10) {
                    parsed_command1[cmdpos] = (char)0;
                    alldone = true;
                }
                else if ((int) cmd_buffer[i] == 32) {
                    parsed_command1[cmdpos] = (char)0;
                    i++;
                    done = true;
                }
                else {
                    parsed_command1[cmdpos] = tolower(cmd_buffer[i]);
                    i++;
                    cmdpos++;
                }
            }
            cmdpos = 0;
            done = false;
            while(i < 32 && !done && !alldone) {
                if ((int) cmd_buffer[i] == 10) {
                    parsed_command2[cmdpos] = (char)0;
                    alldone = true;
                }
                else if ((int) cmd_buffer[i] == 32) {
                    parsed_command2[cmdpos] = (char)0;
                    i++;
                    done = true;
                }
                else {
                    parsed_command2[cmdpos] = tolower(cmd_buffer[i]);
                    i++;
                    cmdpos++;
                }
            }
            // ensure all strings are null-terminated before we use strcmp
            parsed_command0[31] = (char)0;
            parsed_command1[31] = (char)0;
            parsed_command2[31] = (char)0;

            if (strcmp(parsed_command0, "?") == 0) {
                display_help();
            }
            else if (strlen(parsed_command0) == 0 || strcmp(parsed_command0, "s") == 0 || strcmp(parsed_command0, "r") == 0) {
                if (strcmp(parsed_command0, "r") == 0) {
                    instr_to_run = RUN_FOREVER; 
                }
                else if (strlen(parsed_command1) > 0) {
                    is_valid = parse_long(parsed_command1, &hex1);
                    if (!is_valid) {
                        printf("Invalid commend %s\nInvalid number of instructions %s\n", cmd_buffer, parsed_command1);
                        instr_to_run = 0;
                    }
                    else {
                        instr_to_run = (int) hex1;
                    }
                }
                else {
                    instr_to_run = 1;
                }
                keep_running = true;
                for (i = 0; ((instr_to_run == RUN_FOREVER || i < instr_to_run) && keep_running); i++) {
                    keep_running = cycle_cpu8080(&(motherboard.cpu), &num_states);
                    if (keep_running) {
                        (*total_states) = (*total_states) + num_states;
                        (*total_instructions) ++;
                        if (is_breakpoint(breakpoint_list, motherboard.cpu.pc, breakpoint_list_size)) {
                            keep_running = false;
                        }
                    }
                }
            }
            else if (strcmp(parsed_command0, "x") == 0) {
                is_valid = parse_long(parsed_command1, &hex1);
                if (!is_valid) {
                    printf("Invalid command %s\nInvalid memory address %s\n", cmd_buffer, parsed_command1);
                }
                if (strlen(parsed_command2) == 0) {
                    hex2 = 1;
                }
                else {
                    is_valid = parse_long(parsed_command2, &hex2);
                    if(!is_valid) {
                        printf("Invalid command %s\nInvalid number of bytes %s\n", cmd_buffer, parsed_command2);
                    }
                }
                for(i=0; i<hex2; i++){
                    printf("%04X: 0x%02X\n", (uint16_t)hex1 + i, motherboard.memory[hex1 + i]);
                }
            }
            else if (strcmp(parsed_command0, "set") == 0) {
                if (strlen(parsed_command1) == 0 || strlen(parsed_command2) == 0) {
                    printf("Invalid command %s\n'set' takes two arguments.\n", cmd_buffer);
                }
                else {
                    hex1 = 0;
                    hex2 = 0;
                    is_valid = parse_long(parsed_command1, &hex1);
                    if (!is_valid || hex1 < 0 || hex1 > 0xFFFF) {
                        printf("Invalid command %s\nfirst argument to set must be a valid address.\n", cmd_buffer);
                    }
                    else {
                        is_valid = parse_long(parsed_command2, &hex2);
                        if (!is_valid || hex2 < 0 || hex2 > 0xFF) {
                            printf("Invalid command %s\nsecond argument to set must be a valid 8-bit value.\n", cmd_buffer);
                        }
                        else {
                            motherboard.memory[hex1] = (uint8_t)hex2;
                        }
                    }
                }
            }
            else if (strcmp(parsed_command0, "b") == 0) {
                if (strlen(parsed_command1) == 0) {
                    set_breakpoint(breakpoint_list, motherboard.cpu.pc, breakpoint_list_size);
                }
                else {
                    is_valid = parse_long(parsed_command1, &hex1);
                    if (!is_valid) {
                        printf("Invalid command %s\nInvalid breakpoint address %s\n", cmd_buffer, parsed_command1);
                    }
                    else {
                        set_breakpoint(breakpoint_list, (int)hex1, breakpoint_list_size);
                    }
                }
            }
            else if ((strcmp(parsed_command0, "info") == 0) && (strcmp(parsed_command1, "b") == 0)) {
                print_breakpoints(breakpoint_list, breakpoint_list_size);
            }
            else if (strcmp(parsed_command0, "bt") == 0) {
                if (motherboard.cpu.sp == motherboard.cpu.stack_pointer_start) {
                    printf("Stack empty\n\n");
                }
                else {
                    printf("Stack Addr & Value\n");
                    printf("------------------\n");
                    for(i = motherboard.cpu.sp; i < motherboard.cpu.stack_pointer_start; i = i + 2) {
                        printf("0x%04X\t0x%02X%02X\n", i, motherboard.memory[i+1], motherboard.memory[i]);
                    }
                    printf("\n");
                }
            }
            else if (strcmp(parsed_command0, "q") == 0) {
                running = false;
                retval = false;
            }
            else if (strcmp(parsed_command0, "rr") == 0) {
                running = false;
                retval = true;
            }
            else {
                printf("Invalid command: %s\n", cmd_buffer);
            }
        }
    }
    return retval;
}