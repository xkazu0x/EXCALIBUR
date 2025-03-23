#include "excalibur.h"

// internal void
// render_gradient(game_bitmap *buffer, game_state *state) {
//     u8 *row = (u8 *)buffer->memory;
//     for (s32 y = 0; y < buffer->size.y; ++y) {
//         u32 *pixel = (u32 *)row;
//         for (s32 x = 0; x < buffer->size.x; ++x) {
//             u8 blue = (x + state->offset.x);
//             u8 green = (y + state->offset.y);
//             u8 red = 0;

//             *pixel++ = ((red << 16) | (green << 16) | blue);
//         }
//         row += buffer->pitch;
//     }
// }

inline s32
round_f32_to_s32(f32 f) {
    s32 result = (s32)(f + 0.5f);
    return(result);
}

inline u32
round_f32_to_u32(f32 f) {
    u32 result = (u32)(f + 0.5f);
    return(result);
}

inline u32
truncate_f32_to_s32(f32 f) {
    u32 result = (s32)f;
    return(result);
}

internal void
draw_rectangle(game_bitmap *bitmap, vec2f min, vec2f max, vec3f color) {
    vec2i mini = vec2i_create(0, 0);
    vec2i maxi = vec2i_create(0, 0);
    
    mini.x = round_f32_to_s32(min.x);
    mini.y = round_f32_to_s32(min.y);
    
    maxi.x = round_f32_to_s32(max.x);
    maxi.y = round_f32_to_s32(max.y);

    if (mini.x < 0) mini.x = 0;
    if (mini.y < 0) mini.y = 0;
    
    if (maxi.x > bitmap->size.x) maxi.x = bitmap->size.x;
    if (maxi.y > bitmap->size.y) maxi.y = bitmap->size.y;

    u32 out_color = ((round_f32_to_u32(color.r * 255.0f) << 16) |
                     (round_f32_to_u32(color.g * 255.0f) << 8) |
                     (round_f32_to_u32(color.b * 255.0f) << 0));
    
    u8 *row = ((u8 *)bitmap->memory +
                 (mini.x * bitmap->bytes_per_pixel) +
                 (mini.y * bitmap->pitch));
    
    for (s32 y = mini.y; y < maxi.y; y++) {
        u32 *pixel = (u32 *)row;
        for (s32 x = mini.x; x < maxi.x; ++x) {
            *pixel++ = out_color;
        }
        row += bitmap->pitch;
    }
}

inline u32
get_tile_value(tile_map *tile_map, vec2i index) {
    u32 result = tile_map->tiles[index.y*tile_map->tile_count.x + index.x];
    return(result);
}

inline tile_map *
get_tile_map(world_map *world, vec2i index) {
    tile_map *result = 0;
    if ((index.x >= 0) && (index.x < world->tile_map_count.x) &&
        (index.y >= 0) && (index.y < world->tile_map_count.y)) {
        result = &world->tile_maps[index.y*world->tile_map_count.x + index.x];
    }
    return(result);
}

internal b32
is_tile_map_point_empty(tile_map *tile_map, vec2f point) {
    b32 result = EX_FALSE;
    
    vec2i tile_point = vec2i_create(truncate_f32_to_s32((point.x - tile_map->offset.x) / tile_map->tile_size),
                                    truncate_f32_to_s32((point.y - tile_map->offset.y) / tile_map->tile_size));
    
    if ((tile_point.x >= 0) && (tile_point.x < tile_map->tile_count.x) &&
        (tile_point.y >= 0) && (tile_point.y < tile_map->tile_count.y)) {
        u32 tile_map_value = get_tile_value(tile_map, tile_point);
        result = (tile_map_value == 0);
    }
    return(result);
}

internal b32
is_world_point_empty(world_map *world, vec2i tile_map_index, vec2f point) {
    b32 result = EX_FALSE;

    tile_map *tile_map = get_tile_map(world, tile_map_index);
    if (tile_map) {
        vec2i tile_point = vec2i_create(truncate_f32_to_s32((point.x - tile_map->offset.x) / tile_map->tile_size),
                                        truncate_f32_to_s32((point.y - tile_map->offset.y) / tile_map->tile_size));
    
        if ((tile_point.x >= 0) && (tile_point.x < tile_map->tile_count.x) &&
            (tile_point.y >= 0) && (tile_point.y < tile_map->tile_count.y)) {
            u32 tile_map_value = get_tile_value(tile_map, tile_point);
            result = (tile_map_value == 0);
        }
    }
    return(result);
}

extern "C" GAME_UPDATE_AND_RENDER(game_update_and_render) {
    // NOTE(xkazu0x): initialize
    EX_ASSERT(sizeof(game_state) <= memory->permanent_storage_size);

#define TILE_MAP_COUNT_X 17
#define TILE_MAP_COUNT_Y 9
    u32 tiles00[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] = {
        {1, 1, 1, 1,  1, 1, 1, 1,  1,  1, 1, 1, 1,  1, 1, 1, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 0, 0, 0,  1, 1, 1, 0,  0,  0, 1, 1, 1,  0, 0, 0, 0},
        {1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 1, 1, 1,  1, 1, 1, 1,  0,  1, 1, 1, 1,  1, 1, 1, 1},
    };
    
    u32 tiles01[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] = {
        {1, 1, 1, 1,  1, 1, 1, 1,  0,  1, 1, 1, 1,  1, 1, 1, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 0, 0, 0,  1, 0, 0, 0,  0,  0, 0, 0, 1,  0, 0, 0, 1},
        {1, 0, 0, 0,  1, 0, 0, 0,  1,  0, 0, 0, 1,  0, 0, 0, 0},
        {1, 0, 0, 0,  1, 0, 0, 0,  0,  0, 0, 0, 1,  0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 1, 1, 1,  1, 1, 1, 1,  1,  1, 1, 1, 1,  1, 1, 1, 1},
    };

    u32 tiles10[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] = {
        {1, 1, 1, 1,  1, 1, 1, 1,  1,  1, 1, 1, 1,  1, 1, 1, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 0, 0, 0,  1, 0, 0, 0,  0,  0, 0, 0, 1,  0, 0, 0, 1},
        {0, 0, 0, 0,  1, 0, 0, 0,  1,  0, 0, 0, 1,  0, 0, 0, 1},
        {1, 0, 0, 0,  1, 0, 0, 0,  0,  0, 0, 0, 1,  0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 1, 1, 1,  1, 1, 1, 1,  0,  1, 1, 1, 1,  1, 1, 1, 1},
    };
    
    u32 tiles11[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] = {
        {1, 1, 1, 1,  1, 1, 1, 1,  0,  1, 1, 1, 1,  1, 1, 1, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 0, 0, 0,  1, 0, 0, 0,  0,  0, 0, 0, 1,  0, 0, 0, 1},
        {0, 0, 0, 0,  1, 0, 0, 0,  1,  0, 0, 0, 1,  0, 0, 0, 1},
        {1, 0, 0, 0,  1, 0, 0, 0,  0,  0, 0, 0, 1,  0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 1, 1, 1,  1, 1, 1, 1,  1,  1, 1, 1, 1,  1, 1, 1, 1},
    };

    tile_map tile_maps[2][2];
    tile_maps[0][0].tile_count = vec2i_create(TILE_MAP_COUNT_X, TILE_MAP_COUNT_Y);
    tile_maps[0][0].offset = vec2f_create(-30, 0);
    tile_maps[0][0].tile_size = 60.0f;
    tile_maps[0][0].tiles = (u32 *)tiles00;

    tile_maps[0][1] = tile_maps[0][0];
    tile_maps[0][1].tiles = (u32 *)tiles01;
    
    tile_maps[1][0] = tile_maps[0][0];
    tile_maps[1][0].tiles = (u32 *)tiles10;
    
    tile_maps[1][1] = tile_maps[0][0];
    tile_maps[1][1].tiles = (u32 *)tiles11;

    tile_map *tile_map = &tile_maps[0][0];

    world_map world;
    world.tile_map_count = vec2i_create(2, 2);
    world.tile_maps = tile_map;
    
    vec2f player_size = vec2f_create(tile_map->tile_size * 0.75f, tile_map->tile_size);
    vec2f player_delta = vec2f_create(0.0f, 0.0f);
    
    game_state *state = (game_state *)memory->permanent_storage;
    if (!memory->initialized) {
        // char *filename = __FILE__;
        // debug_file_handle file = memory->debug_platform_read_file(thread, filename);
        // if (file.memory) {
        //     memory->debug_platform_write_file(thread, "test.out", file.size, file.memory);
        //     memory->debug_platform_free_file(thread, file.memory);
        // }
        state->player_pos = vec2f_create(tile_map->tile_size * 2, tile_map->tile_size * 2);
        memory->initialized = EX_TRUE;
    }
    
    // NOTE(xkazu0x): update
    if (input->keyboard[KK_W].down) {
        player_delta.y = -1.0f;
    }
    if (input->keyboard[KK_S].down) {
        player_delta.y = 1.0f;
    }
    if (input->keyboard[KK_A].down) {
        player_delta.x = -1.0f;
    }
    if (input->keyboard[KK_D].down) {
        player_delta.x = 1.0f;
    }
    
    player_delta = player_delta * 128.0f;
    vec2f new_player_pos = state->player_pos + player_delta * clock->delta;
    if (is_tile_map_point_empty(tile_map, new_player_pos) &&
        is_tile_map_point_empty(tile_map, {new_player_pos.x - player_size.x*0.5f, new_player_pos.y}) &&
        is_tile_map_point_empty(tile_map, {new_player_pos.x + player_size.x*0.5f, new_player_pos.y})) {
        state->player_pos = new_player_pos;
    }
    
    // NOTE(xkazu0x): render
    draw_rectangle(bitmap,
                   vec2f_create(0.0f, 0.0f),
                   vec2f_create(bitmap->size.x, bitmap->size.y),
                   vec3f_create(0.2f, 0.3f, 0.3f));
    
    for (s32 row = 0; row < 9; row++) {
        for (s32 column = 0; column < 17; column++) {
            u32 tile = get_tile_value(tile_map, vec2i_create(column, row));
            f32 gray = 0.5f;
            if (tile == 1) {
                gray = 1.0f;
            }
            vec2f min = vec2f_create(tile_map->offset.x + column * tile_map->tile_size, tile_map->offset.y + row * tile_map->tile_size);
            vec2f max = vec2f_create(min.x + tile_map->tile_size, min.y + tile_map->tile_size);
            draw_rectangle(bitmap, min, max, vec3f_create(gray, gray, gray));
        }
    }
    
    vec2f player_min = vec2f_create(state->player_pos.x - 0.5f*player_size.x, state->player_pos.y - player_size.y);
    vec2f player_max = vec2f_create(player_min.x + player_size.x, player_min.y + player_size.y);
    draw_rectangle(bitmap, player_min, player_max, vec3f_create(1.0f, 1.0f, 0.0f));
}
