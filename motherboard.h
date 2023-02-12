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
    Mix_Chunk *sound_shot;
    Mix_Chunk *sound_flash_player_die;
    Mix_Chunk *sound_invader_die;
    Mix_Chunk *sound_extended_play;
    Mix_Chunk *sound_fleet_movement_1;
    Mix_Chunk *sound_fleet_movement_2;
    Mix_Chunk *sound_fleet_movement_3;
    Mix_Chunk *sound_fleet_movement_4;
    Mix_Chunk *sound_ufo_hit;

    bool credit_pressed;
    bool one_player_start_pressed;
    bool two_player_start_pressed;
    bool player_one_left_pressed;
    bool player_one_fire_pressed;
    bool player_one_right_pressed;
    bool player_two_left_pressed;
    bool player_two_fire_pressed;
    bool player_two_right_pressed;

    // DIPs 3 and 5 are read as two bits - 00 is 3 ships, 01 is 4, 10 is 5, 11 is 6
    bool dip3;
    bool dip5;
    // DIP 6 controls whether extra ship is at 1500 (if 0) or 1000 (if 1)
    bool dip6;
    // DIP 7 controls whether coin info is displayed in the demo screen, 0 = on
    bool dip7;

    // Shift register
    // See: https://www.computerarcheology.com/Arcade/SpaceInvaders/Hardware.html#dedicated-shift-hardware
    uint16_t shift_register;
    uint8_t shift_register_offset;

    SDL_Renderer *renderer;
    SDL_Window *window; 
} spaceinvaders_motherboard8080;

void init_test_motherboard(motherboard8080 *motherboard);
void init_space_invaders_motherboard(spaceinvaders_motherboard8080 *motherboard);
void destroy_motherboard(motherboard8080 *motherboard);
void destroy_spaceinvaders_motherboard(spaceinvaders_motherboard8080 *motherboard);
void spaceinvaders_screen_clear(spaceinvaders_motherboard8080 *motherboard);
void spaceinvaders_screen_draw(spaceinvaders_motherboard8080 *motherboard);

#endif