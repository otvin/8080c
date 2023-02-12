#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include "memory.h"
#include "disassembler.h"
#include "cpu8080.h"
#include "motherboard.h"
#include "debugger.h"


int main(int argc, char *argv[]) {

    uint64_t total_states, num_states, cur_states, total_instructions;
    double sec;
    bool run, debug_mode = false;
    clock_t start_time, end_time, diff;
    struct timeval start_time1, end_time1;
    double sec1;
    uint16_t ignore;
    SDL_Event event;

    if (argc > 1) {
        if (strncmp(argv[1], "-debug", 6) == 0) {
            debug_mode = true;
        }
    }


    total_states = 0;
    total_instructions = 0;

    spaceinvaders_motherboard8080 motherboard;
    cpu8080 cpu;
    init_cpu8080(&cpu);
    init_space_invaders_motherboard(&motherboard);
    


    start_time = clock();
    gettimeofday(&start_time1, NULL);
    run = true;

    if (debug_mode) {
        run = debug_8080((motherboard8080 *) &motherboard, &cpu, &total_states, &total_instructions);
    }
    while (run && (!cpu.halted)) {
        
        while (SDL_PollEvent(&event)) {
            switch(event.type) {
                case SDL_QUIT:
                    run = false;
                    break;
                case SDL_KEYDOWN:
                    switch(event.key.keysym.sym) {
                        case SDLK_0:
                            motherboard.credit_pressed = true;
                            break;
                        case SDLK_1:
                            motherboard.one_player_start_pressed = true;
                            break;
                        case SDLK_2:
                            motherboard.two_player_start_pressed = true;
                            break;
                        case SDLK_LEFT:
                            motherboard.player_one_left_pressed = true;
                            motherboard.player_two_left_pressed = true;
                            break;
                        case SDLK_RIGHT:
                            motherboard.player_one_right_pressed = true;
                            motherboard.player_two_right_pressed = true;
                            break;
                        case SDLK_SPACE:
                            motherboard.player_one_fire_pressed = true;
                            motherboard.player_two_fire_pressed = true;
                            break;
                    }
                    break;
                case SDL_KEYUP:
                    switch(event.key.keysym.sym) {
                        case SDLK_0:
                            motherboard.credit_pressed = false;
                            break;
                        case SDLK_1:
                            motherboard.one_player_start_pressed = false;
                            break;
                        case SDLK_2:
                            motherboard.two_player_start_pressed = false;
                            break;
                        case SDLK_LEFT:
                            motherboard.player_one_left_pressed = false;
                            motherboard.player_two_left_pressed = false;
                            break;
                        case SDLK_RIGHT:
                            motherboard.player_one_right_pressed = false;
                            motherboard.player_two_right_pressed = false;
                            break;
                        case SDLK_SPACE:
                            motherboard.player_one_fire_pressed = false;
                            motherboard.player_two_fire_pressed = false;
                            break;
                    }
                    break;
            }
        }

        cur_states = 0;
        while (run && cur_states <= 16667) {
            run = cycle_cpu8080((motherboard8080 *) &motherboard, &cpu, &num_states);
            if (run) {
                cur_states += num_states;
                total_states += num_states;
                total_instructions ++;
            }
            else {
                debug_8080((motherboard8080 *) &motherboard, &cpu, &total_states, &total_instructions);
            }
        }
        if (run) {
            // first interrupt
            do_interrupt((motherboard8080 *) &motherboard, &cpu, 1, &ignore);
        }
        while (run && cur_states <= 33333) {
            run = cycle_cpu8080((motherboard8080 *) &motherboard, &cpu, &num_states);
            if (run) {
                cur_states += num_states;
                total_states += num_states;
                total_instructions ++;
            }
            else {
                debug_8080((motherboard8080 *) &motherboard, &cpu, &total_states, &total_instructions);
            }
        }
        if (run) {
            // second interrupt
            do_interrupt((motherboard8080 *) &motherboard, &cpu, 2, &ignore);
        }
        spaceinvaders_screen_draw(&motherboard);

        // insert loop here to delay
    }
    end_time = clock();
    gettimeofday(&end_time1, NULL);
    diff = end_time - start_time;
    sec =  ((double)diff) / ((double)CLOCKS_PER_SEC);
    sec1 = ((double)(end_time1.tv_usec - start_time1.tv_usec) / 1000000) + ((double)(end_time1.tv_sec - start_time1.tv_sec));

    printf("Duration in CPU time: %f sec\n", sec);
    printf("Duration in clock time: %f sec\n", sec1);
    printf("Num instructions: %ld\n", total_instructions);
    if (sec > 0) {
        printf("Performance: %f states per CPU second\n", ((double)total_states) / sec);
    }
    if (sec1 > 0) {
        printf("Performance: %f states per clock second\n", ((double)total_states) / sec1);
    }

    destroy_spaceinvaders_motherboard(&motherboard);
    return EXIT_SUCCESS;
}