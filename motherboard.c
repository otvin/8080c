#include <stdio.h>
#include <stdbool.h>
#include "motherboard.h"
#include "memory.h"


bool handle_test_output(motherboard8080 *motherboard, uint8_t port, uint8_t out) {
    if (port == 0x0) {
        printf("%c", (char) out);
        return(true);
    }
    else {
        printf("Output port %02X not handled.", port);
        return(false);
    }
}

bool handle_test_input(motherboard8080 *motherboard, uint8_t port, uint8_t *in) {
    // the 8080 CPU tests do not involve any input.
    printf("Input port %02X not handled.", port);
    return(false);
}

void init_test_motherboard(motherboard8080 *motherboard) {
    // motherboard for the 8080 test programs
    motherboard->memory = init_memory(0x10000);
    motherboard->input_handler = &handle_test_input;
    motherboard->output_handler = &handle_test_output;
}

bool handle_space_invaders_output(motherboard8080 *motherboard, uint8_t port, uint8_t out) {
    spaceinvaders_motherboard8080 *real_motherboard;
    uint16_t tmp16;
    int channel;

    real_motherboard = (spaceinvaders_motherboard8080 *)motherboard;

    switch(port) {
        case 0x2: 
            real_motherboard->shift_register_offset = out & 0x7;
            break;
        case 0x3:
            /*  https://www.computerarcheology.com/Arcade/SpaceInvaders/Hardware.html#output
                bits 0-4 are sounds
                bit 0 = UFO (repeats)
                bit 1 = Shot
                bit 2 = Flash (player die)
                bit 3 = Invader die
                bit 4 = Extended Play
                bit 5 = AMP enable
                bit 6, 7 = NC (not wired)
            
                Only bit will be set.  Bit 5 is a no-op as it had a function with the analog
                sound in the original machine.
            */
            switch(out) {
                case 0x1:
                    channel = Mix_PlayChannel(-1, real_motherboard->sound_ufo, 0);
                    break;
                case 0x2:
                    channel = Mix_PlayChannel(-1, real_motherboard->sound_shot, 0);
                    break;
                case 0x4:
                    channel = Mix_PlayChannel(-1, real_motherboard->sound_flash_player_die, 0);
                    break;
                case 0x8:
                    channel = Mix_PlayChannel(-1, real_motherboard->sound_invader_die, 0);
                    break;
                case 0x10:
                    channel = Mix_PlayChannel(-1, real_motherboard->sound_extended_play, 0);
                    break;
                case 0x20:
                    // no-op, but set channel so that we can do the test properly below
                    channel = 0;
                    break;
            }
            if (channel == -1) {
                printf("Unable to play WAV file: %s\n", Mix_GetError());
            }
            // while(Mix_Playing(channel) != 0);  <--- used to test sounds
        case 0x4:
            real_motherboard->shift_register = real_motherboard->shift_register >> 8;
            tmp16 = out;
            real_motherboard->shift_register = real_motherboard->shift_register | (tmp16 << 8);
            break;
        case 0x5:
            /* 
                https://www.computerarcheology.com/Arcade/SpaceInvaders/Hardware.html#output
                bits 0-4 are sounds
                bits 0-3 = Fleet movement 1-4
                bit 4 = UFO hit
                bit 5 = flip screen in Cocktail mode - not implemented in this version
                bits 6, 7 = NC (not wired)
            */
            switch(out) {
                case 0x1:
                    channel = Mix_PlayChannel(-1, real_motherboard->sound_fleet_movement_1, 0);
                    break;
                case 0x2:
                    channel = Mix_PlayChannel(-1, real_motherboard->sound_fleet_movement_2, 0);
                    break;
                case 0x4:
                    channel = Mix_PlayChannel(-1, real_motherboard->sound_fleet_movement_3, 0);
                    break;
                case 0x8:
                    channel = Mix_PlayChannel(-1, real_motherboard->sound_fleet_movement_4, 0);
                    break;
                case 0x10:
                    channel = Mix_PlayChannel(-1, real_motherboard->sound_ufo_hit, 0);
                    break;
            }
            if (channel == -1) {
                printf("Unable to play WAV file: %s\n", Mix_GetError());
            }
        case 0x6:
            /*
                https://www.reddit.com/r/EmuDev/comments/rykj04/questions_about_watchdog_port_in_space_invaders/
                Watchdog resets the entire machine if port 6 doesn't receive read/write requests every so many cycles.
                For the purposes of our emulator, we can ignore it.
            */
           break;
        default:
            printf("Output port %02X not handled.", port);
            return(false);
    }
    return(true);
}

bool handle_space_invaders_input(motherboard8080 *motherboard, uint8_t port, uint8_t *in) {
    spaceinvaders_motherboard8080 *real_motherboard;
    real_motherboard = (spaceinvaders_motherboard8080 *)motherboard;
    uint16_t tmp16;

    /*
        see: https://www.walkofmind.com/programming/side/hardware.htm
        Keep in mind - a bit for an input is enabled until they are processed by the CPU and then the
        bit is disabled
        Only 3 input ports are used
        1 - Coin slot, start game and player 1 controls
        2 - Game configuration and player 2 controls
        3 - Shift register
    */
    switch(port) {
        case 0x1:
            *in = 0x08;  // bit 3 is always pressed per computerarchaeology.com
            if (real_motherboard->credit_pressed) {
                *in |= 0x01;
            }
            if (real_motherboard->two_player_start_pressed) {
                *in |= 0x02;
            }
            if (real_motherboard->one_player_start_pressed) {
                *in |= 0x04;
            }
            if (real_motherboard->player_one_fire_pressed) {
                *in |= 0x10;
            }
            if (real_motherboard->player_one_left_pressed) {
                *in |= 0x20;
            }
            if (real_motherboard->player_one_right_pressed) {
                *in |= 0x40;
            }
            // bit 7 is ignored
            break;
        case 0x2:
            *in = 0x0;
            if (real_motherboard->dip3) {
                *in |= 0x01;
            }
            if (real_motherboard->dip5) {
                *in |= 0x02;
            }
            if (real_motherboard->dip6) {
                *in |= 0x08;
            }
            if (real_motherboard->player_two_fire_pressed) {
                *in |= 0x10;
            }
            if (real_motherboard->player_two_left_pressed) {
                *in |= 0x20;
            }
            if (real_motherboard->player_two_right_pressed) {
                *in |= 0x40;
            }
            if (real_motherboard->dip7) {
                *in |= 0x80;
            }
            break;
        case 0x3:
            tmp16 = real_motherboard->shift_register;
            tmp16 = (tmp16 >> (8 - real_motherboard->shift_register_offset)) & 0xFF;
            *in = (uint8_t) tmp16;
            break;
        default:
            printf("Input port %02X not handled.", port);
            return(false);
    }
    return(true);
}

void init_space_invaders_motherboard(spaceinvaders_motherboard8080 *motherboard) {
    motherboard->base.memory = init_memory(0x4000);

    load_rom("invaders.h", 0x0000, motherboard->base.memory);
    load_rom("invaders.g", 0x0800, motherboard->base.memory);
    load_rom("invaders.f", 0x1000, motherboard->base.memory);
    load_rom("invaders.e", 0x1800, motherboard->base.memory);

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0){
        printf("Unable to initialize SDL: %s\n", SDL_GetError());
    }
    if (Mix_OpenAudio(22050, AUDIO_S16SYS, 2, 4096) != 0) {
        printf("Unable to initialize audio: %s\n", Mix_GetError());
    }
    motherboard->sound_ufo = Mix_LoadWAV("sounds/ufo_lowpitch.wav");
    if (motherboard->sound_ufo == NULL) {
        printf("Unable to load WAV file: %s\n", Mix_GetError());
    }
    motherboard->sound_shot = Mix_LoadWAV("sounds/shoot.wav");
    if (motherboard->sound_shot == NULL) {
        printf("Unable to load WAV file: %s\n", Mix_GetError());
    }
    motherboard->sound_flash_player_die = Mix_LoadWAV("sounds/explosion.wav");
    if (motherboard->sound_flash_player_die == NULL) {
        printf("Unable to load WAV file: %s\n", Mix_GetError());
    }
    motherboard->sound_invader_die = Mix_LoadWAV("sounds/invaderkilled.wav");
    if (motherboard->sound_invader_die == NULL) {
        printf("Unable to load WAV file: %s\n", Mix_GetError());
    }
    motherboard->sound_extended_play = Mix_LoadWAV("sounds/extended_play.wav");
    if (motherboard->sound_extended_play == NULL) {
        printf("Unable to load WAV file: %s\n", Mix_GetError());
    }
    motherboard->sound_fleet_movement_1 = Mix_LoadWAV("sounds/fastinvader1.wav");
    if (motherboard->sound_fleet_movement_1 == NULL) {
        printf("Unable to load WAV file: %s\n", Mix_GetError());
    }
    motherboard->sound_fleet_movement_2 = Mix_LoadWAV("sounds/fastinvader2.wav");
    if (motherboard->sound_fleet_movement_2 == NULL) {
        printf("Unable to load WAV file: %s\n", Mix_GetError());
    }
    motherboard->sound_fleet_movement_3 = Mix_LoadWAV("sounds/fastinvader3.wav");
    if (motherboard->sound_fleet_movement_3 == NULL) {
        printf("Unable to load WAV file: %s\n", Mix_GetError());
    }
    motherboard->sound_fleet_movement_4 = Mix_LoadWAV("sounds/fastinvader4.wav");
    if (motherboard->sound_fleet_movement_4 == NULL) {
        printf("Unable to load WAV file: %s\n", Mix_GetError());
    }
    motherboard->sound_ufo_hit = Mix_LoadWAV("sounds/ufo_highpitch.wav");
    if (motherboard->sound_ufo_hit == NULL) {
        printf("Unable to load WAV file: %s\n", Mix_GetError());
    }

    SDL_CreateWindowAndRenderer(224, 256, 0, &(motherboard->window), &(motherboard->renderer));
    spaceinvaders_screen_clear(motherboard);
    
    motherboard->base.input_handler = &handle_space_invaders_input;
    motherboard->base.output_handler = &handle_space_invaders_output;

    motherboard->credit_pressed = false;
    motherboard->one_player_start_pressed = false;
    motherboard->two_player_start_pressed = false;
    motherboard->player_one_left_pressed = false;
    motherboard->player_one_fire_pressed = false;
    motherboard->player_one_right_pressed = false;
    motherboard->player_two_left_pressed = false;
    motherboard->player_two_fire_pressed = false;
    motherboard->player_two_right_pressed = false;

    motherboard->dip3 = true;
    motherboard->dip5 = false;
    motherboard->dip6 = true;
    motherboard->dip7 = false;

    motherboard->shift_register = 0x0000;
    motherboard->shift_register_offset = 0x0; 
}

void destroy_motherboard(motherboard8080 *motherboard) {
    destroy_memory(&(motherboard->memory));
}

void destroy_spaceinvaders_motherboard(spaceinvaders_motherboard8080 *motherboard) {
    Mix_FreeChunk(motherboard->sound_ufo);
    Mix_FreeChunk(motherboard->sound_shot);
    Mix_FreeChunk(motherboard->sound_flash_player_die);
    Mix_FreeChunk(motherboard->sound_invader_die);
    Mix_FreeChunk(motherboard->sound_extended_play);
    Mix_FreeChunk(motherboard->sound_fleet_movement_1);
    Mix_FreeChunk(motherboard->sound_fleet_movement_2);
    Mix_FreeChunk(motherboard->sound_fleet_movement_3);
    Mix_FreeChunk(motherboard->sound_fleet_movement_4);
    Mix_FreeChunk(motherboard->sound_ufo_hit);

    SDL_DestroyRenderer(motherboard->renderer);
    SDL_DestroyWindow(motherboard->window);
    destroy_motherboard(&(motherboard->base));
}

void spaceinvaders_screen_clear(spaceinvaders_motherboard8080 *motherboard) {
    SDL_SetRenderDrawColor(motherboard->renderer, 0, 0, 0, 0);
    SDL_RenderClear(motherboard->renderer);
}

void spaceinvaders_screen_draw(spaceinvaders_motherboard8080 *motherboard) {
    /* see http://computerarcheology.com/Arcade/SpaceInvaders/Hardware.html or
       https://www.walkofmind.com/programming/side/hardware.htm for some descriptions of the screen geometry
    */
    uint8_t x = 0;
    uint8_t y = 255;
    uint16_t mem_pos = 0x2400; // start of VRAM
    uint8_t byte;
    
    byte = motherboard->base.memory[mem_pos];
    
    while (mem_pos <= 0x3FFF) {
        if ((byte >> (7 - (y % 8))) & 0x01) {
            SDL_SetRenderDrawColor(motherboard->renderer, 255, 255, 255, 255);
        }
        else {
            SDL_SetRenderDrawColor(motherboard->renderer, 0, 0, 0, 255);
        }
        SDL_RenderDrawPoint(motherboard->renderer, x, y);
        y -= 1;
        if (y == 255) {
            // it underflowed
            x += 1;
        }
        if (y % 8 == 0) {
            mem_pos ++;
            byte = motherboard->base.memory[mem_pos];
        }
    }
    
    SDL_RenderPresent(motherboard->renderer);
}
    