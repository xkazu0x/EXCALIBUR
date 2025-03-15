#include "excalibur.h"

internal void
render_gradient(game_bitmap *buffer, game_state *state) {
    u8 *row = (u8 *)buffer->memory;
    for (s32 y = 0; y < buffer->size.y; ++y) {
        u32 *pixel = (u32 *)row;
        for (s32 x = 0; x < buffer->size.x; ++x) {
            u8 blue = (x + state->offset.x);
            u8 green = (y + state->offset.y);
            u8 red = 0;

            *pixel++ = ((red << 16) | (green << 16) | blue);
        }
        row += buffer->pitch;
    }
}

internal void
render_player(game_bitmap *buffer, vec2i pos) {
    u8 *end_of_buffer = (u8 *)buffer->memory + buffer->pitch * buffer->size.y;
    
    u32 color = 0xFFFFFFFF;
    u32 left = pos.x;
    u32 right = pos.x + 16;
    u32 top = pos.y;
    u32 bottom = pos.y + 16;
    
    for (u32 x = left; x < right; ++x) {
        u8 *pixel = ((u8 *)buffer->memory +
                     (x * buffer->bytes_per_pixel) +
                     (top * buffer->pitch));
        for (u32 y = top; y < bottom; ++y) {
            if ((pixel >= buffer->memory) &&
                ((pixel + 4) < end_of_buffer)) {
                *(u32 *)pixel = color;
            }
            pixel += buffer->pitch;
        }
    }
}

GAME_UPDATE_AND_RENDER(game_update_and_render) {
    // NOTE(xkazu0x): initialize
    EX_ASSERT(sizeof(game_state) <= memory->permanent_storage_size);
    game_state *state = (game_state *)memory->permanent_storage;
    if (!memory->initialized) {
        char *filename = __FILE__;
        debug_file_handle file = memory->debug_platform_read_file(thread, filename);
        if (file.memory) {
            memory->debug_platform_write_file(thread, "test.out", file.size, file.memory);
            memory->debug_platform_free_file(thread, file.memory);
        }

        state->offset = vec2i_create(0, 0);
        state->player_pos = vec2i_create(100, 100);
        
        memory->initialized = EX_TRUE;
    }

    // NOTE(xkazu0x): update
    gamepad_input *gamepad = &input->gamepads[0];
    
    state->offset.x += gamepad->left_stick.x;
    state->offset.y -= gamepad->left_stick.y;

    state->player_pos.x += gamepad->left_stick.x;
    state->player_pos.y -= gamepad->left_stick.y;
    
    if (input->keyboard[KK_LEFT].down) {
        state->offset.x--;
        state->player_pos.x--;
    }
    if (input->keyboard[KK_RIGHT].down) {
        state->offset.x++;
        state->player_pos.x++;
    }
    if (input->keyboard[KK_UP].down) {
        state->offset.y--;
        state->player_pos.y--;
    }
    if (input->keyboard[KK_DOWN].down) {
        state->offset.y++;
        state->player_pos.y++;
    }

    // NOTE(xkazu0x): render
    render_gradient(bitmap, state);
    render_player(bitmap, state->player_pos);
    render_player(bitmap, input->mouse.position);

    for (u32 i = 0; i < MB_MAX; i++) {
        u32 render_size = 16;
        u32 offset = 4;
        if (input->mouse.buttons[i].down) {
            render_player(bitmap, vec2i_create(offset + i * (render_size + offset), offset));
        }
    }
}
