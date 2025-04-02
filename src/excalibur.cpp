#include "excalibur.h"

////////////////////////////////
// TODO(xkazu0x): temp functions
internal inline s32
round_f32_to_s32(f32 f) {
    s32 result = (s32)(f + 0.5f);
    return(result);
}

internal inline u32
round_f32_to_u32(f32 f) {
    u32 result = (u32)(f + 0.5f);
    return(result);
}

internal inline s32
truncate_f32_to_s32(f32 f) {
    s32 result = (s32)f;
    return(result);
}

internal inline u32
truncate_f32_to_u32(f32 f) {
    u32 result = (u32)f;
    return(result);
}

internal inline s32
floor_f32_to_s32(f32 f) {
    s32 result = (s32)floorf(f);
    return(result);
}
////////////////////////////////

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
get_tile_value_unchecked(world_map *world, tile_chunk *tile_chunk, u32 tile_x, u32 tile_y) {
    EX_ASSERT(tile_chunk);
    EX_ASSERT(tile_x < world->chunk_dim);
    EX_ASSERT(tile_y < world->chunk_dim);

    u32 result = tile_chunk->tiles[tile_y*world->chunk_dim + tile_x];
    return(result);
}

internal b32
get_tile_chunk_tile_value(world_map *world, tile_chunk *tile_chunk, u32 tile_x, u32 tile_y) {
    u32 result = 0;

    if (tile_chunk) {    
        result = get_tile_value_unchecked(world, tile_chunk, tile_x, tile_y);
    }
    
    return(result);
}

inline tile_chunk *
get_tile_chunk(world_map *world, s32 tile_chunk_x, s32 tile_chunk_y) {
    tile_chunk *result = 0;
    
    if ((tile_chunk_x >= 0) && (tile_chunk_x < world->tile_chunk_count_x) &&
        (tile_chunk_y >= 0) && (tile_chunk_y < world->tile_chunk_count_y)) {
        result = &world->tile_chunks[tile_chunk_y*world->tile_chunk_count_x + tile_chunk_x];
    }
    
    return(result);
}

inline tile_chunk_position
get_tile_chunk_position(world_map *world, u32 abs_tile_x, u32 abs_tile_y) {
    tile_chunk_position result;

    result.tile_chunk_x = abs_tile_x >> world->chunk_shift;
    result.tile_chunk_y = abs_tile_y >> world->chunk_shift;
    result.tile_x = abs_tile_x & world->chunk_mask;
    result.tile_y = abs_tile_y & world->chunk_mask;
    
    return(result);
}

internal b32
get_tile_value(world_map *world, u32 tile_x, u32 tile_y) {
    tile_chunk_position chunk_pos = get_tile_chunk_position(world, tile_x, tile_y);
    tile_chunk *tile_chunk = get_tile_chunk(world, chunk_pos.tile_chunk_x, chunk_pos.tile_chunk_y);
    u32 result = get_tile_chunk_tile_value(world, tile_chunk, chunk_pos.tile_x, chunk_pos.tile_y);
    
    return(result);
}

internal b32
is_world_point_empty(world_map *world, world_position world_pos) {
    u32 tile_value = get_tile_value(world, world_pos.tile_x, world_pos.tile_y);
    b32 result = (tile_value == 0);
    
    return(result);
}

inline void
recanonicalize_coord(world_map *world, u32 *tile_index, f32 *tile_rel) {
    // TODO(xkazu0x): floorf does not fucking work
    s32 offset = floor_f32_to_s32(*tile_rel / world->tile_size_in_meters);
    *tile_index += offset;
    *tile_rel -= offset*world->tile_size_in_meters;

    EX_ASSERT(*tile_rel >= 0);
    EX_ASSERT(*tile_rel <= world->tile_size_in_meters);
}

inline world_position
recanonicalize_position(world_map *world, world_position pos) {
    world_position result = pos;
    recanonicalize_coord(world, &result.tile_x, &result.tile_rel.x);
    recanonicalize_coord(world, &result.tile_y, &result.tile_rel.y);
    return(result);
}

extern "C" GAME_UPDATE_AND_RENDER(game_update_and_render) {
    EX_ASSERT(sizeof(game_state) <= memory->permanent_storage_size);

#define TILE_CHUNK_COUNT_X 256
#define TILE_CHUNK_COUNT_Y 256
    u32 temp_tiles[TILE_CHUNK_COUNT_Y][TILE_CHUNK_COUNT_X] = {
        {1, 1, 1, 1,  1, 1, 1, 1,  1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1,  1, 1, 1, 1,  1, 1, 1, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1,  1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1,  1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1,  1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1,  1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1,  1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1,  1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 1, 1, 1,  1, 1, 1, 1,  0,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  0,  1, 1, 1, 1,  1, 1, 1, 1},
        {1, 1, 1, 1,  1, 1, 1, 1,  0,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  0,  1, 1, 1, 1,  1, 1, 1, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1,  1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1,  1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1,  1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1,  1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1,  1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1,  1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 1, 1, 1,  1, 1, 1, 1,  1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1,  1, 1, 1, 1,  1, 1, 1, 1},
    };

    world_map world;
    // NOTE(xkazu0x): this is set to using 256x256 tile chunks
    world.chunk_shift = 8;
    world.chunk_mask = (1 << world.chunk_shift) - 1;
    world.chunk_dim = 256;

    world.tile_size_in_meters = 1.4f;
    world.tile_size_in_pixels = 60;
    world.meters_to_pixels = (f32)world.tile_size_in_pixels / (f32)world.tile_size_in_meters;

    world.tile_chunk_count_x = 1;
    world.tile_chunk_count_y = 1;

    tile_chunk tile_chunks;
    tile_chunks.tiles = (u32 *)temp_tiles;
    world.tile_chunks = &tile_chunks;

    vec2f player_size = vec2f_create(world.tile_size_in_meters*0.75f, world.tile_size_in_meters);
    vec2f player_delta = vec2f_create(0.0f, 0.0f);
        
    game_state *state = (game_state *)memory->permanent_storage;
    if (!memory->initialized) {
        // char *filename = __FILE__;
        // debug_file_handle file = memory->debug_platform_read_file(thread, filename);
        // if (file.memory) {
        //     memory->debug_platform_write_file(thread, "test.out", file.size, file.memory);
        //     memory->debug_platform_free_file(thread, file.memory);
        // }
        state->player_pos.tile_x = 3;
        state->player_pos.tile_y = 3;
        state->player_pos.tile_rel = vec2f_create(5.0f);
        
        memory->initialized = EX_TRUE;
    }
    
    // NOTE(xkazu0x): update
    if (input->keyboard[KEY_W].down) {
        player_delta.y = 1.0f;
    }
    if (input->keyboard[KEY_S].down) {
        player_delta.y = -1.0f;
    }
    if (input->keyboard[KEY_A].down) {
        player_delta.x = -1.0f;
    }
    if (input->keyboard[KEY_D].down) {
        player_delta.x = 1.0f;
    }
    
    player_delta = player_delta*3.0f;

    world_position new_player_pos = state->player_pos;
    new_player_pos.tile_rel = new_player_pos.tile_rel + player_delta*clock->delta_seconds;
    new_player_pos = recanonicalize_position(&world, new_player_pos);
    
    world_position player_left = new_player_pos;
    player_left.tile_rel.x = player_left.tile_rel.x - player_size.x*0.5;
    player_left = recanonicalize_position(&world, player_left);
    
    world_position player_right = new_player_pos;
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
                   vec2f_create(bitmap->size.x, bitmap->size.y),
                   vec3f_create(0.2f, 0.3f, 0.3f));

    f32 center_x = 0.5f*((f32)bitmap->size.x);
    f32 center_y = 0.5f*((f32)bitmap->size.y);
    
    for (s32 rel_row = -10; rel_row < 10; rel_row++) {
        for (s32 rel_column = -20; rel_column < 20; rel_column++) {
            u32 column = state->player_pos.tile_x + rel_column;
            u32 row = state->player_pos.tile_y + rel_row;
            u32 tile_value = get_tile_value(&world, column, row);
            
            f32 gray = 0.5f;
            if (tile_value == 1) {
                gray = 1.0f;
            }

            if ((column == state->player_pos.tile_x) &&
                (row == state->player_pos.tile_y)) {
                gray = 0.0f;
            }

            f32 min_x = center_x + ((f32)rel_column)*world.tile_size_in_pixels;
            f32 min_y = center_y - ((f32)rel_row)*world.tile_size_in_pixels;
            f32 max_x = min_x + world.tile_size_in_pixels;
            f32 max_y = min_y - world.tile_size_in_pixels;
            draw_rectangle(bitmap, {min_x, max_y}, {max_x, min_y}, vec3f_create(gray, gray, gray));
        }
    }

    f32 player_left_dim = center_x + world.meters_to_pixels*state->player_pos.tile_rel.x - 0.5f*world.meters_to_pixels*player_size.x;
    f32 player_right_dim = center_y - world.meters_to_pixels*state->player_pos.tile_rel.y - world.meters_to_pixels*player_size.y;
    vec2f player_min = vec2f_create(player_left_dim, player_right_dim);
    vec2f player_max = player_min + player_size*world.meters_to_pixels;
    draw_rectangle(bitmap, player_min, player_max, vec3f_create(1.0f, 1.0f, 0.0f));
}
