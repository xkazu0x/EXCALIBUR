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
#include "excalibur_random.h"
#include "excalibur_tile.cpp"

internal void
draw_rectangle(os_framebuffer_t *framebuffer, vec2f min, vec2f max, vec3f color) {
    vec2i mini = vec2i_create(0, 0);
    vec2i maxi = vec2i_create(0, 0);
    
    mini.x = round_f32_to_s32(min.x);
    mini.y = round_f32_to_s32(min.y);
    
    maxi.x = round_f32_to_s32(max.x);
    maxi.y = round_f32_to_s32(max.y);

    if (mini.x < 0) mini.x = 0;
    if (mini.y < 0) mini.y = 0;
    
    if (maxi.x > framebuffer->width) maxi.x = framebuffer->width;
    if (maxi.y > framebuffer->height) maxi.y = framebuffer->height;

    u32 out_color = ((round_f32_to_u32(color.r * 255.0f) << 16) |
                     (round_f32_to_u32(color.g * 255.0f) << 8) |
                     (round_f32_to_u32(color.b * 255.0f) << 0));
    
    u8 *row = ((u8 *)framebuffer->pixels +
                 (mini.x * framebuffer->bytes_per_pixel) +
                 (mini.y * framebuffer->pitch));
    for (s32 y = mini.y; y < maxi.y; y++) {
        u32 *pixel = (u32 *)row;
        for (s32 x = mini.x; x < maxi.x; ++x) {
            *pixel++ = out_color;
        }
        row += framebuffer->pitch;
    }
}

internal void
draw_bitmap(os_framebuffer_t *framebuffer, bitmap_t *bitmap, f32 xf, f32 yf) {
    s32 x_min = round_f32_to_s32(xf);
    s32 y_min = round_f32_to_s32(yf);
    s32 x_max = round_f32_to_s32(xf + (f32)bitmap->width);
    s32 y_max = round_f32_to_s32(yf + (f32)bitmap->height);
    
    if (x_min < 0) x_min = 0;
    if (y_min < 0) y_min = 0;
    if (x_max > framebuffer->width) x_max = framebuffer->width;
    if (y_max > framebuffer->height) y_max = framebuffer->height;

    // TODO(xkazu0x): source_row needs to be changed based on cliping
    u32 *source_row = bitmap->pixels + bitmap->width*(bitmap->height - 1);
    u8 *dest_row = ((u8 *)framebuffer->pixels +
                    x_min*framebuffer->bytes_per_pixel +
                    y_min*framebuffer->pitch);
    
    for (s32 y = y_min; y < y_max; y++) {
        u32 *dest = (u32 *)dest_row;
        u32 *source = source_row;
        for (s32 x = x_min; x < x_max; x++) {
            f32 alpha = (f32)((*source >> 24) & 0xFF)/255.0f;
            f32 src_red = (f32)((*source >> 16) & 0xFF);
            f32 src_green = (f32)((*source >> 8) & 0xFF);
            f32 src_blue = (f32)((*source >> 0) & 0xFF);

            f32 dest_red = (f32)((*dest >> 16) & 0xFF);
            f32 dest_green = (f32)((*dest >> 8) & 0xFF);
            f32 dest_blue = (f32)((*dest >> 0) & 0xFF);
            
            f32 red = (1.0f-alpha)*dest_red + alpha*src_red;
            f32 green = (1.0f-alpha)*dest_green + alpha*src_green;
            f32 blue = (1.0f-alpha)*dest_blue + alpha*src_blue;

            *dest = (((u32)(red + 0.5f) << 16) |
                     ((u32)(green + 0.5f) << 8) |
                     ((u32)(blue + 0.5f) << 0));
            
            ++dest;
            ++source;
        }
        dest_row += framebuffer->pitch;
        source_row -= bitmap->width;
    }
}

#pragma pack(push, 1)
struct bitmap_header_t {
    u16 file_type;
    u32 file_size;
    u16 reserved1;
    u16 reserved2;
    u32 bitmap_offset;
    u32 size;
    s32 width;
    s32 height;
    u16 planes;
    u16 bits_per_pixel;
    u32 compression;
    u32 size_of_bitmap;
    s32 horz_resolution;
    s32 vert_resolution;
    u32 colors_used;
    u32 colors_important;

    u32 red_mask;
    u32 green_mask;
    u32 blue_mask;
};
#pragma pack(pop)

internal bitmap_t
debug_load_bitmap(debug_os_read_file_t *debug_os_read_file, os_thread_t *thread, char *filename) {
    bitmap_t result = {};
    
    debug_file_t file = debug_os_read_file(thread, filename);
    if (file.size != 0) {
        bitmap_header_t *header = (bitmap_header_t *)file.data;
        u32 *pixels = (u32 *)((u8 *)file.data + header->bitmap_offset);
        result.pixels = pixels;
        result.width = header->width;
        result.height = header->height;

        EX_ASSERT(header->compression == 3);
        
        // NOTE(xkazu0x): byte order in memory is determined bu the header itself,
        // so we have to read out the masks and convert the pixels ourselves.
        u32 red_mask = header->red_mask;
        u32 green_mask = header->green_mask;
        u32 blue_mask = header->blue_mask;
        u32 alpha_mask = ~(red_mask | green_mask | blue_mask);
        
        bit_scan_result_t red_shift = find_least_significant_set_bit(red_mask);
        bit_scan_result_t green_shift = find_least_significant_set_bit(green_mask);
        bit_scan_result_t blue_shift = find_least_significant_set_bit(blue_mask);
        bit_scan_result_t alpha_shift = find_least_significant_set_bit(alpha_mask);

        EX_ASSERT(red_shift.found);
        EX_ASSERT(green_shift.found);
        EX_ASSERT(blue_shift.found);
        EX_ASSERT(alpha_shift.found);
        
        u32 *source_dest = pixels;
        for (s32 y = 0; y < header->height; y++) {
            for (s32 x = 0; x < header->width; x++) {
                u32 c = *source_dest;
                *source_dest = ((((c >> alpha_shift.index) & 0xFF) << 24) |
                                (((c >> red_shift.index) & 0xFF) << 16) |
                                (((c >> green_shift.index) & 0xFF) << 8) |
                                (((c >> blue_shift.index) & 0xFF) << 0));
                ++source_dest;
            }
        }
    }
    
    return(result);
}

extern "C" GAME_UPDATE_AND_RENDER(game_update_and_render) {
    EX_ASSERT(sizeof(game_state_t) <= memory->permanent_storage_size);
    game_state_t *state = (game_state_t *)memory->permanent_storage;
    
    if (!memory->initialized) {
        state->test_bitmap = debug_load_bitmap(memory->debug_os_read_file,
                                               thread, "res/spritesheet.bmp");
        state->test_bitmap1 = debug_load_bitmap(memory->debug_os_read_file,
                                                thread, "res/cross.bmp");
        
        state->player_pos.tile_x = 1;
        state->player_pos.tile_y = 3;
        state->player_pos.tile_offset_x = 0.0f;
        state->player_pos.tile_offset_y = 0.0f;

        state->world_arena = memory_arena_create(memory->permanent_storage_size - sizeof(game_state_t),
                                               (u8 *)memory->permanent_storage + sizeof(game_state_t));
        state->world = (world_t *)memory_arena_push(&state->world_arena, sizeof(world_t));
        
        world_t *world = state->world;
        world->tile_map = (tile_map_t *)memory_arena_push(&state->world_arena, sizeof(tile_map_t));
        
        tile_map_t *tile_map = world->tile_map;
        tile_map->chunk_shift = 4;
        tile_map->chunk_mask = (1 << tile_map->chunk_shift) - 1;
        tile_map->chunk_dim = (1 << tile_map->chunk_shift);
        
        tile_map->tile_chunk_count_x = 128;
        tile_map->tile_chunk_count_y = 128;
        tile_map->tile_chunk_count_z = 2;
        tile_map->tile_chunks = (tile_chunk_t *)memory_arena_push(&state->world_arena,
                                                                  (tile_map->tile_chunk_count_x*
                                                                   tile_map->tile_chunk_count_y*
                                                                   tile_map->tile_chunk_count_z)*
                                                                  sizeof(tile_chunk_t));
        
        tile_map->tile_size_in_meters = 1.4f;
        u32 random_number_index = 0;
        
        u32 tile_count_x = 15;
        u32 tile_count_y = 10;
        u32 screen_x = 0;
        u32 screen_y = 0;
        
        u32 abs_tile_z = 0;
                    
        b32 door_left = EX_FALSE;
        b32 door_right = EX_FALSE;
        b32 door_top = EX_FALSE;
        b32 door_bottom = EX_FALSE;

        b32 door_up = EX_FALSE;
        b32 door_down = EX_FALSE;
        
        for (u32 screen_index = 0; screen_index < 100; screen_index++) {
            // TODO(xkazu0x): random number generator
            EX_ASSERT(random_number_index < EX_ARRAY_COUNT(random_number_table));
            u32 random_choice;
            if (door_up || door_down) {
                random_choice = random_number_table[random_number_index++] % 2;
            } else {
                random_choice = random_number_table[random_number_index++] % 3;
            }

            b32 created_z_door = EX_FALSE;
            if (random_choice == 2) {
                created_z_door = EX_TRUE;
                if (abs_tile_z == 0) {
                    door_up = EX_TRUE;
                } else {
                    door_down = EX_TRUE;
                }
            } else if (random_choice == 1) {
                door_right = EX_TRUE;
            } else {
                door_top = EX_TRUE;
            }
            
            for (u32 tile_y = 0; tile_y < tile_count_y; tile_y++) {
                for (u32 tile_x = 0; tile_x < tile_count_x; tile_x++) {
                    u32 abs_tile_x = screen_x*tile_count_x + tile_x;
                    u32 abs_tile_y = screen_y*tile_count_y + tile_y;
                    
                    u32 tile_value = 1;
                    if ((tile_x == 0) && (!door_left || (tile_y != (tile_count_y/2)))) {
                        tile_value = 2;
                    }
                    
                    if ((tile_x == (tile_count_x - 1)) && (!door_right || (tile_y != (tile_count_y/2)))) {
                        tile_value = 2;
                    }
                    
                    if ((tile_y == 0) && (!door_bottom || (tile_x != (tile_count_x/2)))) {
                        tile_value = 2;
                    }
                    
                    if ((tile_y == (tile_count_y - 1)) && (!door_top || (tile_x != (tile_count_x/2)))) {
                        tile_value = 2;
                    }

                    if ((tile_x == 10) && (tile_y == 6)) {
                        if (door_up) {
                            tile_value = 3;
                        }
                        
                        if (door_down) {
                            tile_value = 4;
                        }
                    }
                        
                    set_tile_map_tile_value(&state->world_arena, tile_map, abs_tile_x, abs_tile_y, abs_tile_z, tile_value);
                }
            }

            door_left = door_right;
            door_bottom = door_top;

            if (created_z_door) {
                door_down = !door_down;
                door_up = !door_up;
            } else {
                door_up = EX_FALSE;
                door_down = EX_FALSE;
            }
            
            door_right = EX_FALSE;
            door_top = EX_FALSE;

            if (random_choice == 2) {
                if (abs_tile_z == 0) {
                    abs_tile_z = 1;
                } else {
                    abs_tile_z = 0;
                }
            } else if (random_choice == 1) {
                screen_x++;
            } else {
                screen_y++;
            }
        }
                
        memory->initialized = EX_TRUE;
    }

    world_t *world = state->world;
    tile_map_t *tile_map = world->tile_map;

    s32 tile_size_in_pixels = 16*4;
    f32 meters_to_pixels = (f32)tile_size_in_pixels / (f32)tile_map->tile_size_in_meters;
    
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
    new_player_pos.tile_offset_x = new_player_pos.tile_offset_x + player_delta.x*clock->delta_seconds;
    new_player_pos.tile_offset_y = new_player_pos.tile_offset_y + player_delta.y*clock->delta_seconds;
    new_player_pos = recanonicalize_position(tile_map, new_player_pos);
    
    tile_map_position_t player_left = new_player_pos;
    player_left.tile_offset_x = player_left.tile_offset_x - player_size.x*0.5;
    player_left = recanonicalize_position(tile_map, player_left);
    
    tile_map_position_t player_right = new_player_pos;
    player_right.tile_offset_x = player_right.tile_offset_x + player_size.x*0.5;
    player_right = recanonicalize_position(tile_map, player_right);
    
    if (is_tile_map_point_empty(tile_map, new_player_pos) &&
        is_tile_map_point_empty(tile_map, player_left) &&
        is_tile_map_point_empty(tile_map, player_right)) {
        if (!are_on_the_same_tile(&state->player_pos, &new_player_pos)) {
            u32 new_tile_value = get_tile_map_tile_value(tile_map, new_player_pos);
            
            if (new_tile_value == 3) {
                ++new_player_pos.tile_z;
            } else if (new_tile_value == 4) {
                --new_player_pos.tile_z;
            }
        }
        
        state->player_pos = new_player_pos;
    }

    // NOTE(xkazu0x): render
    draw_rectangle(framebuffer,
                   vec2f_create(0.0f),
                   vec2f_create(framebuffer->width, framebuffer->height),
                   vec3f_create(0.2f, 0.3f, 0.3f));

    draw_bitmap(framebuffer, &state->test_bitmap, 0, 0);
    
    f32 screen_center_x = 0.5f*((f32)framebuffer->width);
    f32 screen_center_y = 0.5f*((f32)framebuffer->height);
    
    for (s32 rel_row = -5; rel_row < 6; rel_row++) {
        for (s32 rel_column = -7; rel_column < 8; rel_column++) {
            u32 column = state->player_pos.tile_x + rel_column;
            u32 row = state->player_pos.tile_y + rel_row;
            
            u32 tile_value = get_tile_map_tile_value(tile_map, column, row, state->player_pos.tile_z);

            vec3f tile_color = vec3f_create(0.5f);
            if (tile_value > 1) {
                if (tile_value == 2) {
                    tile_color = vec3f_create(1.0f);
                }

                if (tile_value > 2) {
                    tile_color = vec3f_create(0.25f);
                }
                
                if ((column == state->player_pos.tile_x) &&
                    (row == state->player_pos.tile_y)) {
                    tile_color = vec3f_create(0.0f);
                }
            // } else {
            //     tile_color = vec3f_create(1.0f, 0.5f, 0.2f);
            // }
            
                f32 center_x = screen_center_x - meters_to_pixels*state->player_pos.tile_offset_x + ((f32)rel_column)*tile_size_in_pixels;
                f32 center_y = screen_center_y + meters_to_pixels*state->player_pos.tile_offset_y - ((f32)rel_row)*tile_size_in_pixels;
                f32 min_x = center_x - 0.5f*tile_size_in_pixels;
                f32 min_y = center_y - 0.5f*tile_size_in_pixels;
                f32 max_x = center_x + 0.5f*tile_size_in_pixels;
                f32 max_y = center_y + 0.5f*tile_size_in_pixels;
                draw_rectangle(framebuffer, {min_x, min_y}, {max_x, max_y}, tile_color);
            }
        }
    }

    f32 player_left_dim = screen_center_x - 0.5f*meters_to_pixels*player_size.x;
    f32 player_right_dim = screen_center_y - meters_to_pixels*player_size.y;
    vec2f player_min = vec2f_create(player_left_dim, player_right_dim);
    vec2f player_max = player_min + player_size*meters_to_pixels;
    draw_rectangle(framebuffer, player_min, player_max, vec3f_create(1.0f, 1.0f, 0.0f));

    draw_bitmap(framebuffer, &state->test_bitmap1, player_min.x, player_min.y);
}
