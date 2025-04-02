#include "excalibur_base.h"
#include "excalibur_intrinsics.h"
#include "excalibur_math.h"
#include "excalibur_log.h"

#include "excalibur_base.cpp"
#include "excalibur_intrinsics.cpp"
#include "excalibur_math.cpp"
#include "excalibur_log.cpp"

#include "excalibur_os.h"

////////////////////////////////////////////////
// NOTE(xkazu0x): exclusive includes

#include "excalibur_game.h"
#include "excalibur_tile.cpp"

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

internal memory_arena_t
memory_arena_init(memi size, u8 *base) {
    memory_arena_t result;
    result.size = size;
    result.used = 0;
    result.base = base;
    return(result);
}

internal void *
memory_arena_push(memory_arena_t *arena, memi size) {
    EX_ASSERT((arena->used + size) <= arena->size);
    void *result = arena->base + arena->used;
    arena->used += size;
    return(result);
}

#define MEMA_PUSH_STRUCT(arena, type) (type *)memory_arena_push(arena, sizeof(type))
#define MEMA_PUSH_ARRAY(arena, count, type) (type *)memory_arena_push(arena, (count)*sizeof(type))

extern "C" GAME_UPDATE_AND_RENDER(game_update_and_render) {
    EX_ASSERT(sizeof(game_state_t) <= memory->permanent_storage_size);
    game_state_t *state = (game_state_t *)memory->permanent_storage;
    
    if (!memory->initialized) {
        state->player_pos.tile_x = 1;
        state->player_pos.tile_y = 3;
        state->player_pos.tile_rel_x = 0.0f;
        state->player_pos.tile_rel_y = 0.0f;

        state->world_arena = memory_arena_init(memory->permanent_storage_size - sizeof(game_state_t),
                                               (u8 *)memory->permanent_storage + sizeof(game_state_t));
        state->world = MEMA_PUSH_STRUCT(&state->world_arena, world_t);
        
        world_t *world = state->world;
        world->tile_map = MEMA_PUSH_STRUCT(&state->world_arena, tile_map_t);
        
        tile_map_t *tile_map = world->tile_map;
        tile_map->chunk_shift = 4;
        tile_map->chunk_mask = (1 << tile_map->chunk_shift) - 1;
        tile_map->chunk_dim = (1 << tile_map->chunk_shift);

        tile_map->tile_size_in_meters = 1.4f;
        tile_map->tile_size_in_pixels = 60;
        tile_map->meters_to_pixels = (f32)tile_map->tile_size_in_pixels / (f32)tile_map->tile_size_in_meters;
        
        tile_map->tile_chunk_count_x = 128;
        tile_map->tile_chunk_count_y = 128;
        tile_map->tile_chunks = MEMA_PUSH_ARRAY(&state->world_arena,
                                                tile_map->tile_chunk_count_x*tile_map->tile_chunk_count_y,
                                                tile_chunk_t);
        
        for (u32 y = 0; y < tile_map->tile_chunk_count_y; y++) {
            for (u32 x = 0; x < tile_map->tile_chunk_count_x; x++) {
                tile_map->tile_chunks[y*tile_map->tile_chunk_count_x + x].tiles =
                    MEMA_PUSH_ARRAY(&state->world_arena, tile_map->chunk_dim*tile_map->chunk_dim, u32);
            }
        }
        
        u32 tile_count_x = 17;
        u32 tile_count_y = 9;
        for (u32 screen_y = 0; screen_y < 32; screen_y++) {
            for (u32 screen_x = 0; screen_x < 32; screen_x++) {
                for (u32 tile_y = 0; tile_y < tile_count_y; tile_y++) {
                    for (u32 tile_x = 0; tile_x < tile_count_x; tile_x++) {
                        u32 abs_tile_x = screen_x*tile_count_x + tile_x;
                        u32 abs_tile_y = screen_y*tile_count_y + tile_y;
                        set_tile_map_tile_value(&state->world_arena, tile_map, abs_tile_x, abs_tile_y,
                                                (tile_x == tile_y) && (tile_y % 2) ? 1 : 0);
                    }
                }
            }
        }
                
        memory->initialized = EX_TRUE;
    }

    world_t *world = state->world;
    tile_map_t *tile_map = world->tile_map;
    
    vec2f player_size = vec2f_create(tile_map->tile_size_in_meters*0.75f, tile_map->tile_size_in_meters);
    vec2f player_delta = vec2f_create(0.0f, 0.0f);

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

    f32 player_speed = 3.0f;
    if (input->keyboard[KEY_SHIFT].down) {
        player_speed = 10.0f;
    }
    
    player_delta = player_delta*player_speed;

    tile_map_position_t new_player_pos = state->player_pos;
    new_player_pos.tile_rel_x = new_player_pos.tile_rel_x + player_delta.x*clock->delta_seconds;
    new_player_pos.tile_rel_y = new_player_pos.tile_rel_y + player_delta.y*clock->delta_seconds;
    new_player_pos = recanonicalize_position(tile_map, new_player_pos);
    
    tile_map_position_t player_left = new_player_pos;
    player_left.tile_rel_x = player_left.tile_rel_x - player_size.x*0.5;
    player_left = recanonicalize_position(tile_map, player_left);
    
    tile_map_position_t player_right = new_player_pos;
    player_right.tile_rel_x = player_right.tile_rel_x + player_size.x*0.5;
    player_right = recanonicalize_position(tile_map, player_right);
    
    if (is_tile_map_point_empty(tile_map, new_player_pos) &&
        is_tile_map_point_empty(tile_map, player_left) &&
        is_tile_map_point_empty(tile_map, player_right)) {
        state->player_pos = new_player_pos;
    }

    // NOTE(xkazu0x): render
    draw_rectangle(bitmap,
                   vec2f_create(0.0f),
                   vec2f_create(bitmap->size.x, bitmap->size.y),
                   vec3f_create(0.2f, 0.3f, 0.3f));

    f32 screen_center_x = 0.5f*((f32)bitmap->size.x);
    f32 screen_center_y = 0.5f*((f32)bitmap->size.y);
    
    for (s32 rel_row = -10; rel_row < 10; rel_row++) {
        for (s32 rel_column = -20; rel_column < 20; rel_column++) {
            u32 column = state->player_pos.tile_x + rel_column;
            u32 row = state->player_pos.tile_y + rel_row;
            u32 tile_value = get_tile_map_tile_value(tile_map, column, row);
            
            f32 gray = 0.5f;
            if (tile_value == 1) {
                gray = 1.0f;
            }

            if ((column == state->player_pos.tile_x) &&
                (row == state->player_pos.tile_y)) {
                gray = 0.0f;
            }

            f32 center_x = screen_center_x - tile_map->meters_to_pixels*state->player_pos.tile_rel_x + ((f32)rel_column)*tile_map->tile_size_in_pixels;
            f32 center_y = screen_center_y + tile_map->meters_to_pixels*state->player_pos.tile_rel_y - ((f32)rel_row)*tile_map->tile_size_in_pixels;
            f32 min_x = center_x - 0.5f*tile_map->tile_size_in_pixels;
            f32 min_y = center_y - 0.5f*tile_map->tile_size_in_pixels;
            f32 max_x = center_x + 0.5f*tile_map->tile_size_in_pixels;
            f32 max_y = center_y + 0.5f*tile_map->tile_size_in_pixels;
            draw_rectangle(bitmap, {min_x, min_y}, {max_x, max_y}, vec3f_create(gray, gray, gray));
        }
    }

    f32 player_left_dim = screen_center_x - 0.5f*tile_map->meters_to_pixels*player_size.x;
    f32 player_right_dim = screen_center_y - tile_map->meters_to_pixels*player_size.y;
    vec2f player_min = vec2f_create(player_left_dim, player_right_dim);
    vec2f player_max = player_min + player_size*tile_map->meters_to_pixels;
    draw_rectangle(bitmap, player_min, player_max, vec3f_create(1.0f, 1.0f, 0.0f));
}
