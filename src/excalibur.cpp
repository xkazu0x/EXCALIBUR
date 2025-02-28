#include "excalibur.h"

GAME_UPDATE_AND_RENDER(game_update_and_render) {
    // NOTE(xkazu0x): initialize
    EX_ASSERT(sizeof(game_state) <= memory->permanent_storage_size);
    game_state *state = (game_state *)memory->permanent_storage;
    if (!memory->initialized) {
        char *filename = __FILE__;
        debug_file_handle file = memory->debug_platform_read_file(filename);
        if (file.memory) {
            memory->debug_platform_write_file("test.out", file.size, file.memory);
            memory->debug_platform_free_file(file.memory);
        }

        state->offset = vec2i_create(0, 0);
        memory->initialized = EX_TRUE;
    }

    // NOTE(xkazu0x): update
    gamepad_input *gamepad = &input->gamepads[0];
    if (input->keyboard[KEY_LEFT].down) state->offset.x--;
    if (input->keyboard[KEY_RIGHT].down) state->offset.x++;
    if (input->keyboard[KEY_UP].down) state->offset.y--;
    if (input->keyboard[KEY_DOWN].down) state->offset.y++;

    if (gamepad->left_stick.x) {
        state->offset.x += gamepad->left_stick.x;
    }
    if (gamepad->left_stick.y) {
        state->offset.y -= gamepad->left_stick.y;
    }
    
    // NOTE(xkazu0x): render
    u8 *row = (u8 *)backbuffer->memory;
    for (s32 y = 0; y < backbuffer->size.y; ++y) {
        u32 *pixel = (u32 *)row;
        for (s32 x = 0; x < backbuffer->size.x; ++x) {
            u8 blue = (x + state->offset.x);
            u8 green = (y + state->offset.y);
            u8 red = 0;

            *pixel++ = ((red << 16) | (green << 8) | blue);
        }
        row += backbuffer->pitch;
    }
}
