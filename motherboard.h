#ifndef MOTHERBOARD_8080_H
#define MOTHERBOARD_8080_H

#include <stdint.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

typedef struct motherboard8080 {
    uint8_t *memory;
    bool (*input_handler)(struct motherboard8080 *motherboard, uint8_t port, uint8_t *in);
    bool (*output_handler)(struct motherboard8080 *motherboard, uint8_t port, uint8_t out);
} motherboard8080;

typedef struct spaceinvaders_motherboard8080 {
    motherboard8080 base;
    Mix_Chunk *sound_ufo; 
} spaceinvaders_motherboard8080;

void init_test_motherboard(motherboard8080 *motherboard);
void init_space_invaders_motherboard(spaceinvaders_motherboard8080 *motherboard);
void destroy_motherboard(motherboard8080 *motherboard);
void destroy_spaceinvaders_motherboard(spaceinvaders_motherboard8080 *motherboard);

#endif