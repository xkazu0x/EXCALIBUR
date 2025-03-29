#include "excalibur.h"

internal void
draw_rectangle(os_bitmap_t *bitmap, vec2f min, vec2f max, vec3f color) {
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
get_tile_value(world_map *world, tile_map *tile_map, vec2i index) {
    EX_ASSERT(tile_map);
    EX_ASSERT((index.x >= 0) && (index.x < world->tile_count.x) &&
              (index.y >= 0) && (index.y < world->tile_count.y));

    u32 result = tile_map->tiles[index.y*world->tile_count.x + index.x];
    return(result);
}

internal b32
is_tile_empty(world_map *world, tile_map *tile_map, vec2i tile) {
    b32 result = EX_FALSE;

    if (tile_map) {    
        if ((tile.x >= 0) && (tile.x < world->tile_count.x) &&
            (tile.y >= 0) && (tile.y < world->tile_count.y)) {
            u32 tile_value = get_tile_value(world, tile_map, tile);
            result = (tile_value == 0);
        }
    }
    
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

inline void
recanonicalize_coord(world_map *world, s32 tile_count, s32 *tile_map_index, s32 *tile_index, f32 *tile_rel) {
    // TODO(xkazu0x): floorf does not fucking work
    s32 offset = floor_f32_to_s32(*tile_rel / world->tile_size_in_meters);
    *tile_index += offset;
    *tile_rel -= offset*world->tile_size_in_meters;

    EX_ASSERT(*tile_rel >= 0);
    // TODO(xkazu0x):
    EX_ASSERT(*tile_rel <= world->tile_size_in_meters);

    if (*tile_index < 0) {
        *tile_index = tile_count + *tile_index;
        --*tile_map_index;
    }

    if (*tile_index >= tile_count) {
        *tile_index = *tile_index - tile_count;
        ++*tile_map_index;
    }
}

inline canonical_position
recanonicalize_position(world_map *world, canonical_position pos) {
    canonical_position result = pos;
    recanonicalize_coord(world, world->tile_count.x, &result.tile_map_index.x, &result.tile_index.x, &result.tile_rel.x);
    recanonicalize_coord(world, world->tile_count.y, &result.tile_map_index.y, &result.tile_index.y, &result.tile_rel.y);
    return(result);
}

internal b32
is_world_point_empty(world_map *world, canonical_position pos) {
    tile_map *tile_map = get_tile_map(world, pos.tile_map_index);
    b32 result = is_tile_empty(world, tile_map, pos.tile_index);
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
    tile_maps[0][0].tiles = (u32 *)tiles00;
    tile_maps[0][1].tiles = (u32 *)tiles10;
    tile_maps[1][0].tiles = (u32 *)tiles01;
    tile_maps[1][1].tiles = (u32 *)tiles11;

    world_map world;
    world.tile_map_count = vec2i_create(2, 2);
    world.tile_count = vec2i_create(TILE_MAP_COUNT_X, TILE_MAP_COUNT_Y);

    world.tile_size_in_meters = 1.4f;
    world.tile_size_in_pixels = 60;
    world.meters_to_pixels = (f32)world.tile_size_in_pixels / (f32)world.tile_size_in_meters;
    
    world.offset = vec2f_create(-(f32)world.tile_size_in_pixels/2, 0.0f);

    world.tile_maps = (tile_map *)tile_maps;
    
    vec2f player_size = vec2f_create(world.tile_size_in_meters*0.75f,
                                     world.tile_size_in_meters);
    vec2f player_delta = vec2f_create(0.0f, 0.0f);
        
    game_state *state = (game_state *)memory->permanent_storage;
    if (!memory->initialized) {
        // char *filename = __FILE__;
        // debug_file_handle file = memory->debug_platform_read_file(thread, filename);
        // if (file.memory) {
        //     memory->debug_platform_write_file(thread, "test.out", file.size, file.memory);
        //     memory->debug_platform_free_file(thread, file.memory);
        // }
        state->player_pos.tile_map_index = vec2i_create(0);
        state->player_pos.tile_index = vec2i_create(3);
        state->player_pos.tile_rel = vec2f_create(5.0f);
        
        memory->initialized = EX_TRUE;
    }
    
    tile_map *tile_map = get_tile_map(&world, state->player_pos.tile_map_index);
    EX_ASSERT(tile_map);
    
    // NOTE(xkazu0x): update
    if (input->keyboard[KEY_W].down) {
        player_delta.y = -1.0f;
    }
    if (input->keyboard[KEY_S].down) {
        player_delta.y = 1.0f;
    }
    if (input->keyboard[KEY_A].down) {
        player_delta.x = -1.0f;
    }
    if (input->keyboard[KEY_D].down) {
        player_delta.x = 1.0f;
    }
    
    player_delta = player_delta*3.0f;

    canonical_position new_player_pos = state->player_pos;
    new_player_pos.tile_rel = new_player_pos.tile_rel + player_delta*clock->delta;
    new_player_pos = recanonicalize_position(&world, new_player_pos);
    
    canonical_position player_left = new_player_pos;
    player_left.tile_rel.x = player_left.tile_rel.x - player_size.x*0.5;
    player_left = recanonicalize_position(&world, player_left);
    
    canonical_position player_right = new_player_pos;
    player_right.tile_rel.x = player_right.tile_rel.x + player_size.x*0.5;
    player_right = recanonicalize_position(&world, player_right);
    
    if (is_world_point_empty(&world, new_player_pos) &&
        is_world_point_empty(&world, player_left) &&
        is_world_point_empty(&world, player_right)) {
        state->player_pos = new_player_pos;
    }

    // NOTE(xkazu0x): render
    draw_rectangle(bitmap,
                   vec2f_create(0.0f),
                   vec2f_from_vec2i(bitmap->size),
                   vec3f_create(0.2f, 0.3f, 0.3f));
    
    for (s32 row = 0; row < 9; row++) {
        for (s32 column = 0; column < 17; column++) {
            u32 tile = get_tile_value(&world, tile_map, vec2i_create(column, row));
            f32 gray = 0.5f;
            if (tile == 1) {
                gray = 1.0f;
            }

            if ((column == state->player_pos.tile_index.x) &&
                (row == state->player_pos.tile_index.y)) {
                gray = 0.0f;
            }
            
            vec2f min = vec2f_create(world.offset.x + ((f32)column)*world.tile_size_in_pixels,
                                     world.offset.y + ((f32)row)*world.tile_size_in_pixels);
            vec2f max = vec2f_create(min.x + world.tile_size_in_pixels,
                                     min.y + world.tile_size_in_pixels);
            draw_rectangle(bitmap, min, max, vec3f_create(gray, gray, gray));
        }
    }

    f32 player_left_dim = world.offset.x + world.tile_size_in_pixels*state->player_pos.tile_index.x + world.meters_to_pixels*state->player_pos.tile_rel.x - 0.5f*world.meters_to_pixels*player_size.x;
    f32 player_right_dim = world.offset.y + world.tile_size_in_pixels*state->player_pos.tile_index.y + world.meters_to_pixels*state->player_pos.tile_rel.y - world.meters_to_pixels*player_size.y;
    vec2f player_min = vec2f_create(player_left_dim, player_right_dim);
    vec2f player_max = player_min + player_size*world.meters_to_pixels;
    draw_rectangle(bitmap, player_min, player_max, vec3f_create(1.0f, 1.0f, 0.0f));
}
