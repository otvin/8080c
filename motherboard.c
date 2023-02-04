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
    int channel;

    real_motherboard = (spaceinvaders_motherboard8080 *)motherboard;
    channel = Mix_PlayChannel(-1, real_motherboard->sound_ufo, 0);
    if (channel == -1) {
        printf("Unable to play WAV file: %s\n", Mix_GetError());
    }
    while(Mix_Playing(channel) != 0);


    printf("Output port %02X not handled.", port);
    return(false);
}

bool handle_space_invaders_input(motherboard8080 *motherboard, uint8_t port, uint8_t *in) {
    spaceinvaders_motherboard8080 *real_motherboard;
    real_motherboard = (spaceinvaders_motherboard8080 *)motherboard;

    printf("Input port %02X not handled.", port);
    return(false);
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


    motherboard->base.input_handler = &handle_space_invaders_input;
    motherboard->base.output_handler = &handle_space_invaders_output;
}

void destroy_motherboard(motherboard8080 *motherboard) {
    destroy_memory(&(motherboard->memory));
}

void destroy_spaceinvaders_motherboard(spaceinvaders_motherboard8080 *motherboard) {
    Mix_FreeChunk(motherboard->sound_ufo);
    destroy_motherboard(&(motherboard->base));
}

