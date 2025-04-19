#include "excalibur_base.h"
#include "excalibur_intrinsics.h"
#include "excalibur_math.h"
#include "excalibur_log.h"

#include "excalibur_base.cpp"
#include "excalibur_intrinsics.cpp"
#include "excalibur_math.cpp"
#include "excalibur_log.cpp"

#include "excalibur_os.h"

////////////////////////////////////
// NOTE(xkazu0x): exclusive includes

#include "excalibur_random.h"
#include "excalibur_world.h"
#include "excalibur_game.h"

#include "excalibur_world.cpp"

internal gamepad_t *
get_gamepad(os_input_t *input, u32 index) {
    EX_ASSERT(index < GAMEPAD_COUNT_MAX);
    gamepad_t *result = 0;
    if (index < GAMEPAD_COUNT_MAX) {
        result = &input->gamepads[index];
    }
    return(result);
}

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
    s32 x_max = x_min + bitmap->width;
    s32 y_max = y_min + bitmap->height;

    s32 source_offset_x = 0;
    s32 source_offset_y = 0;
    
    if (x_min < 0) {
        source_offset_x = -x_min;
        x_min = 0;
    }
    if (y_min < 0) {
        source_offset_y = -y_min;
        y_min = 0;
    }
    if (x_max > framebuffer->width) x_max = framebuffer->width;
    if (y_max > framebuffer->height) y_max = framebuffer->height;

    // TODO(xkazu0x): source_row needs to be changed based on cliping
    u32 *source_row = bitmap->pixels + bitmap->width*(bitmap->height - 1);
    source_row += -source_offset_y*bitmap->width + source_offset_x;
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

internal void
entity_make_low(game_state_t *state, u32 low_index) {
    low_entity_t *low_entity = &state->low_entities[low_index];
    u32 high_index = low_entity->high_entity_index;
    if (high_index) {
        u32 last_high_index = state->high_entity_count - 1;
        if (high_index != last_high_index) {
            high_entity_t *last_entity = state->high_entities_ + last_high_index;
            high_entity_t *del_entity = state->high_entities_ + high_index;
            
            *del_entity = *last_entity;
            state->low_entities[last_entity->low_entity_index].high_entity_index = high_index;
        }
        --state->high_entity_count;
        low_entity->high_entity_index = 0;
    }
}

internal high_entity_t *
entity_make_high(game_state_t *state, u32 low_index) {
    high_entity_t *high_entity = 0;
    low_entity_t *low_entity = state->low_entities + low_index;
    if (low_entity->high_entity_index) {
        high_entity = state->high_entities_ + low_entity->high_entity_index;
    } else {
        if (state->high_entity_count < EX_ARRAY_COUNT(state->high_entities_)) {
            u32 high_index = state->high_entity_count++;
            high_entity = state->high_entities_ + high_index;

            vec3f diff = subtract(state->world, &low_entity->pos, &state->camera_pos);
            high_entity->pos = vec2f{diff.x, diff.y};
            high_entity->d_pos = vec2f{0, 0};
            high_entity->chunk_z = low_entity->pos.chunk_z;
            high_entity->direction = 0;
            high_entity->low_entity_index = low_index;
            
            low_entity->high_entity_index = high_index;
        } else {
            INVALID_CODE_PATH();
        }
    }
    return(high_entity);
}

internal low_entity_t *
entity_get_low(game_state_t *state, u32 index) {
    low_entity_t *result = 0;
    if ((index > 0) && (index < state->low_entity_count)) {
        result = state->low_entities + index;
    }
    return(result);
}

internal entity_t
entity_get_high(game_state_t *state, u32 low_index) {
    entity_t result = {};
    if ((low_index > 0) && (low_index < state->low_entity_count)) {
        result.low_index = low_index;
        result.low = state->low_entities + low_index;
        result.high = entity_make_high(state, low_index);
    }
    return(result);
}

internal u32
entity_add_low(game_state_t *state, entity_type_t type) {
    EX_ASSERT(state->low_entity_count < EX_ARRAY_COUNT(state->low_entities));
    u32 entity_index = state->low_entity_count++;

    state->low_entities[entity_index] = {};
    state->low_entities[entity_index].type = type;

    return(entity_index);
}

internal u32
wall_add(game_state_t *state, u32 tile_x, u32 tile_y, u32 tile_z) {
    u32 entity_index = entity_add_low(state, ENTITY_TYPE_WALL);
    low_entity_t *entity = entity_get_low(state, entity_index);
    
    entity->pos = chunk_position_from_tile_position(state->world, tile_x, tile_y, tile_z);
    entity->height = state->world->tile_side_in_meters;
    entity->width = entity->height;
    entity->collides = EX_TRUE;
    
    return(entity_index);
}

internal u32
player_add(game_state_t *state) {
    u32 entity_index = entity_add_low(state, ENTITY_TYPE_PLAYER);
    low_entity_t *entity = entity_get_low(state, entity_index);
    
    entity->pos = state->camera_pos;
    entity->pos.offset_.x = 0.0f;
    entity->pos.offset_.y = 0.0f;
    entity->height = 0.5f;
    entity->width = 1.0f;
    entity->collides = EX_TRUE;
    
    if (state->camera_following_entity_index == 0) {
        state->camera_following_entity_index = entity_index;
    }
    
    return(entity_index);
}

internal b32
wall_test(f32 wall_x, f32 rel_x, f32 rel_y, f32 player_delta_x, f32 player_delta_y,
          f32 *t_min, f32 min_y, f32 max_y) {
    b32 hit = EX_FALSE;
    
    f32 t_epsilon = 0.01f;
    if (player_delta_x != 0.0f) {
        f32 t_result = (wall_x - rel_x) / player_delta_x;
        f32 y = rel_y + t_result*player_delta_y;
        if ((t_result >= 0.0f) && (*t_min > t_result)) {
            if (y >= min_y && (y <= max_y)) {
                *t_min = EX_MAX(0.0f, t_result - t_epsilon);
                hit = EX_TRUE;
            }
        }
    }
    
    return(hit);
}

internal void
player_move(game_state_t *state, entity_t entity, vec2f dd_pos, f32 delta) {
    world_t *world = state->world;

    f32 dd_pos_length = length_sqr(dd_pos);
    if (dd_pos_length > 1.0f) {
        dd_pos *= (1.0f/sqrt_f32(dd_pos_length));
    }
    
    f32 player_speed = 50.0f; // m/s^2
    dd_pos *= player_speed;
    
    // TODO(xkazu0x): ODE here!
    dd_pos += -5.0f*entity.high->d_pos;
    
    //vec2f old_player_pos = entity.high->pos;
    vec2f player_delta = (0.5f*dd_pos*sqr(delta) + entity.high->d_pos*delta);
    entity.high->d_pos = dd_pos*delta + entity.high->d_pos;
    //vec2f new_player_pos = old_player_pos + player_delta;
    
    /*
    // NOTE(xkazu0x): check collision
    u32 min_tile_x = EX_MIN(old_player_pos.tile_x, new_player_pos.tile_x);
    u32 min_tile_y = EX_MIN(old_player_pos.tile_y, new_player_pos.tile_y);
    u32 max_tile_x = EX_MAX(old_player_pos.tile_x, new_player_pos.tile_x);
    u32 max_tile_y = EX_MAX(old_player_pos.tile_y, new_player_pos.tile_y);

    u32 entity_tile_width = ceil_f32_to_s32(entity->width/tile_map->tile_size_in_meters);
    u32 entity_tile_height = ceil_f32_to_s32(entity->height/tile_map->tile_size_in_meters);
    
    min_tile_x -= entity_tile_width;
    min_tile_y -= entity_tile_height;
    max_tile_x += entity_tile_width;
    max_tile_y += entity_tile_height;
    
    u32 tile_z = entity->pos.tile_z;
    */
    
    for (u32 iteration = 0; iteration < 4; iteration++) {
        f32 t_min = 1.0f;
        vec2f wall_normal = {};
        u32 hit_high_entity_index = 0;

        vec2f desired_pos = entity.high->pos + player_delta;
        
        for (u32 test_high_entity_index = 1;
             test_high_entity_index < state->high_entity_count;
             test_high_entity_index++) {
            if (test_high_entity_index != entity.low->high_entity_index) {
                entity_t test_entity;
                test_entity.high = state->high_entities_ + test_high_entity_index;
                test_entity.low_index = test_entity.high->low_entity_index;
                test_entity.low = state->low_entities + test_entity.low_index;
                
                if (test_entity.low->collides) {
                    f32 dim_w = test_entity.low->width + entity.low->width;
                    f32 dim_h = test_entity.low->width + entity.low->height;
            
                    vec2f min_corner = -0.5f*vec2f{dim_w, dim_h};
                    vec2f max_corner =  0.5f*vec2f{dim_w, dim_h};
                
                    vec2f rel = entity.high->pos - test_entity.high->pos;

                    if (wall_test(min_corner.x, rel.x, rel.y, player_delta.x, player_delta.y,
                                  &t_min, min_corner.y, max_corner.y)) {
                        wall_normal = vec2f{-1.0f, 0.0f};
                        hit_high_entity_index = test_high_entity_index;
                    }
                
                    if (wall_test(max_corner.x, rel.x, rel.y, player_delta.x, player_delta.y,
                                  &t_min, min_corner.y, max_corner.y)) {
                        wall_normal = vec2f{1.0f, 0.0f};
                        hit_high_entity_index = test_high_entity_index;
                    }
                
                    if (wall_test(min_corner.y, rel.y, rel.x, player_delta.y, player_delta.x,
                                  &t_min, min_corner.x, max_corner.x)) {
                        wall_normal = vec2f{0.0f, -1.0f};
                        hit_high_entity_index = test_high_entity_index;
                    }
                
                    if (wall_test(max_corner.y, rel.y, rel.x, player_delta.y, player_delta.x,
                                  &t_min, min_corner.x, max_corner.x)) {
                        wall_normal = vec2f{0.0f, 1.0f};
                        hit_high_entity_index = test_high_entity_index;
                    }
                }
            }
        }
        
        entity.high->pos += t_min*player_delta;
        if (hit_high_entity_index) {
            entity.high->d_pos = entity.high->d_pos - 1*vec2f_dot(entity.high->d_pos, wall_normal)*wall_normal;
            player_delta = desired_pos - entity.high->pos;
            player_delta = player_delta - 1*vec2f_dot(player_delta, wall_normal)*wall_normal;

            high_entity_t *hit_high_entity = state->high_entities_ + hit_high_entity_index;
            low_entity_t *hit_low_entity = state->low_entities + hit_high_entity->low_entity_index;
            // TODO(xkazu0x): stairs
            //entity.high->tile_z += hit_low_entity->d_tile_z;
        } else {
            break;
        }
    }

    /////////////////////////////////////////
    // NOTE(xkazu0x): update facing direction
    // TODO(xkazu0x): change to using the acceleration vector
    if ((entity.high->d_pos.x == 0.0f) && (entity.high->d_pos.y == 0.0f)) {
        // NOTE(xkazu0x): leave direction whatever it was
    } else if (abs_f32(entity.high->d_pos.x) > abs_f32(entity.high->d_pos.y)) {
        if (entity.high->d_pos.x > 0) {
            entity.high->direction = 1;
        } else {
            entity.high->direction = 3;
        }
    } else if (abs_f32(entity.high->d_pos.x) < abs_f32(entity.high->d_pos.y)) {
        if (entity.high->d_pos.y > 0) {
            entity.high->direction = 0;
        } else {
            entity.high->direction = 2;
        }
    }

    entity.low->pos = map_into_tile_space(state->world, state->camera_pos, entity.high->pos);
}

internal void
offset_and_check_frequency_by_area(game_state_t *state,
                                   vec2f offset,
                                   rect2f high_frequency_bounds) {
    // TODO(xkazu0x): clearly this has a bug...
    for (u32 entity_index = 1;
         entity_index < state->high_entity_count;
         ) {
        high_entity_t *high = state->high_entities_ + entity_index;
        high->pos += offset;
        if (is_in_rect2f(high_frequency_bounds, high->pos)) {
            ++entity_index;
        } else {
            entity_make_low(state, entity_index);
        }
    }
}

internal void
camera_set(game_state_t *state, world_position_t new_camera_pos) {
    world_t *world = state->world;
    
    vec3f d_camera_pos = subtract(world, &new_camera_pos, &state->camera_pos);
    state->camera_pos = new_camera_pos;

    u32 tile_span_x = 17*3;
    u32 tile_span_y = 9*3;
    rect2f camera_bounds = rect2f_center_dim(vec2f_create(0.0f),
                                             world->tile_side_in_meters*vec2f_create((f32)tile_span_x,
                                                                                     (f32)tile_span_y));
    vec2f entity_offset_for_frame = -vec2f{d_camera_pos.x, d_camera_pos.y};
    offset_and_check_frequency_by_area(state, entity_offset_for_frame, camera_bounds);

    // TODO(xkazu0x): do this in terms of tile chunks
#if 0
    s32 min_tile_x = new_camera_pos.tile_x - tile_span_x/2;
    s32 max_tile_x = new_camera_pos.tile_x + tile_span_x/2;
    s32 min_tile_y = new_camera_pos.tile_y - tile_span_y/2;
    s32 max_tile_y = new_camera_pos.tile_y + tile_span_y/2;
    for (u32 entity_index = 1;
         entity_index < state->low_entity_count;
         entity_index++) {
        low_entity_t *low = state->low_entities + entity_index;
        if (low->high_entity_index == 0) {
            if ((low->pos.tile_z == new_camera_pos.tile_z) &&
                (low->pos.tile_x >= min_tile_x) &&
                (low->pos.tile_x <= max_tile_x) &&
                (low->pos.tile_y >= min_tile_y) &&
                (low->pos.tile_y <= max_tile_y)) {
                entity_make_high(state, entity_index);
            }
        }
    }
#endif
}

extern "C" GAME_UPDATE_AND_RENDER(game_update_and_render) {
    EX_ASSERT(sizeof(game_state_t) <= memory->permanent_storage_size);
    game_state_t *state = (game_state_t *)memory->permanent_storage;
    
    u32 tile_count_x = 17;
    u32 tile_count_y = 9;
    
    if (!memory->initialized) {
        entity_add_low(state, ENTITY_TYPE_NULL);
        state->high_entity_count = 1;

        state->player_sprites[0] =
            debug_load_bitmap(memory->debug_os_read_file, thread, "res/skull_back.bmp");
        state->player_sprites[1] =
            debug_load_bitmap(memory->debug_os_read_file, thread, "res/skull_right.bmp");
        state->player_sprites[2] =
            debug_load_bitmap(memory->debug_os_read_file, thread, "res/skull_front.bmp");
        state->player_sprites[3] =
            debug_load_bitmap(memory->debug_os_read_file, thread, "res/skull_left.bmp");
        
        state->world_arena = memory_arena_create(memory->permanent_storage_size - sizeof(game_state_t),
                                               (u8 *)memory->permanent_storage + sizeof(game_state_t));
        state->world = (world_t *)memory_arena_push(&state->world_arena, sizeof(world_t));
        world_t *world = state->world;
        world_initialize(world, 1.4f);

        u32 screen_base_x = 0;
        u32 screen_base_y = 0;
        u32 screen_base_z = 0;

        u32 screen_x = screen_base_x;
        u32 screen_y = screen_base_y;
        u32 abs_tile_z = screen_base_z;
        
        u32 random_number_index = 0;
        
        // TODO(xkazu0x): replace all this with real world generation
        b32 door_left = EX_FALSE;
        b32 door_right = EX_FALSE;
        b32 door_top = EX_FALSE;
        b32 door_bottom = EX_FALSE;
        b32 door_up = EX_FALSE;
        b32 door_down = EX_FALSE;        
        for (u32 screen_index = 0; screen_index < 10; screen_index++) {
            // TODO(xkazu0x): random number generator
            EX_ASSERT(random_number_index < EX_ARRAY_COUNT(random_number_table));
            u32 random_choice;
            if (1) { //(door_up || door_down) {
                random_choice = random_number_table[random_number_index++] % 2;
            } else {
                random_choice = random_number_table[random_number_index++] % 3;
            }
            b32 created_z_door = EX_FALSE;
            if (random_choice == 2) {
                created_z_door = EX_TRUE;
                if (abs_tile_z == screen_base_z) {
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
                        
                    //set_world_tile_value(&state->world_arena, world, abs_tile_x, abs_tile_y, abs_tile_z, tile_value);
                    if (tile_value == 2) {
                        wall_add(state, abs_tile_x, abs_tile_y, abs_tile_z);
                    }
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
                if (abs_tile_z == screen_base_z) {
                    abs_tile_z = screen_base_z + 1;
                } else {
                    abs_tile_z = screen_base_z;
                }
            } else if (random_choice == 1) {
                screen_x += 1;
            } else {
                screen_y += 1;
            }
        }

        world_position_t new_camera_pos = {};
        new_camera_pos = chunk_position_from_tile_position(state->world,
                                                           screen_base_x*tile_count_x + tile_count_x/2,
                                                           screen_base_y*tile_count_y + tile_count_y/2,
                                                           screen_base_z);
        camera_set(state, new_camera_pos);
                
        memory->initialized = EX_TRUE;
    }

    world_t *world = state->world;

    s32 tile_size_in_pixels = 16;
    f32 meters_to_pixels = (f32)tile_size_in_pixels / (f32)world->tile_side_in_meters;

    //////////////////////////
    // NOTE(xkazu0x): controls
    for (u32 gamepad_index = 0; gamepad_index < EX_ARRAY_COUNT(input->gamepads); gamepad_index++) {
        gamepad_t *gamepad = get_gamepad(input, gamepad_index);
        u32 low_entity_index = state->player_gamepad_index[gamepad_index];
        if (low_entity_index == 0) {
            if (gamepad->start.pressed) {
                u32 player_entity_index = player_add(state);
                state->player_gamepad_index[gamepad_index] = player_entity_index;
            }
            
            // NOTE(xkazu0x): combine keyboard and gamepad inputs
            // only for the first player
            if (gamepad_index == 1) {
                if (input->keyboard[KEY_SPACE].pressed) {
                    u32 player_entity_index = player_add(state);
                    state->player_gamepad_index[gamepad_index] = player_entity_index;
                }
            }
        } else {
            entity_t controlling_entity = entity_get_high(state, low_entity_index);
            
            vec2f dd_pos = {};
            dd_pos = vec2f{gamepad->left_stick.x, gamepad->left_stick.y};

            // NOTE(xkazu0x): combine keyboard and gamepad inputs
            // only for the first player
            if (gamepad_index == 1) {
                if (input->keyboard[KEY_W].down) {
                    dd_pos.y = 1.0f;
                }
                if (input->keyboard[KEY_S].down) {
                    dd_pos.y = -1.0f;
                }
                if (input->keyboard[KEY_A].down) {
                    dd_pos.x = -1.0f;
                }
                if (input->keyboard[KEY_D].down) {
                    dd_pos.x = 1.0f;
                }
            }
            
            player_move(state, controlling_entity, dd_pos, clock->delta_seconds);
        }
    }
    
    ///////////////////////////////
    // NOTE(xkazu0x): camera update
    vec2f entity_offset_for_frame = {};
    entity_t camera_following_entity = entity_get_high(state, state->camera_following_entity_index);
    if (camera_following_entity.high) {
        world_position_t new_camera_pos = state->camera_pos;
        new_camera_pos.chunk_z = camera_following_entity.low->pos.chunk_z;
        
#if 0
        if (camera_following_entity.high->pos.x > (9.0f*world->tile_side_in_meters)) {
            new_camera_pos.tile_x += 17;
        }
        if (camera_following_entity.high->pos.x < -(9.0f*world->tile_side_in_meters)) {
            new_camera_pos.tile_x -= 17;
        }
        if (camera_following_entity.high->pos.y > (5.0f*world->tile_side_in_meters)) {
            new_camera_pos.tile_y += 9;
        }
        if (camera_following_entity.high->pos.y < -(5.0f*world->tile_side_in_meters)) {
            new_camera_pos.tile_y -= 9;
        }
#else
        new_camera_pos = camera_following_entity.low->pos;
#endif
        
        camera_set(state, new_camera_pos);
    }
    
    ////////////////////////
    // NOTE(xkazu0x): render
    draw_rectangle(framebuffer,
                   vec2f_create(0.0f),
                   vec2f_create(framebuffer->width, framebuffer->height),
                   vec3f_create(0.2f, 0.3f, 0.3f));
    
    f32 screen_center_x = 0.5f*((f32)framebuffer->width);
    f32 screen_center_y = 0.5f*((f32)framebuffer->height);

#if 0
    for (s32 rel_row = -4; rel_row < 5; rel_row++) {
        for (s32 rel_column = -8; rel_column < 9; rel_column++) {
            s32 column = state->camera_pos.tile_x + rel_column;
            s32 row = state->camera_pos.tile_y + rel_row;

            // TODO(xkazu0x): theres is a problem when subtracting the camera pos tile and
            // the rel tile, the result can't find a tile chunk
            u32 tile_value = get_world_tile_value(world, column, row, state->camera_pos.tile_z);

            vec3f tile_color = vec3f_create(0.5f);
            if (tile_value > 0) {
                if (tile_value == 2) {
                    tile_color = vec3f_create(1.0f);
                }

                if (tile_value > 2) {
                    tile_color = vec3f_create(0.25f);
                }
#if 0
                entity_t *entity = &state->entities[1];
                if (entity->exists) {
                    if ((column == entity->pos.tile_x) &&
                        (row == entity->pos.tile_y)) {
                        tile_color = vec3f_create(0.0f);
                    }
                }
#endif
            } else {
                tile_color = vec3f_create(1.0f, 0.5f, 0.2f);
            }
            
            f32 center_x = screen_center_x - meters_to_pixels*state->camera_pos.tile_offset_.x + ((f32)rel_column)*tile_size_in_pixels;
            f32 center_y = screen_center_y + meters_to_pixels*state->camera_pos.tile_offset_.y - ((f32)rel_row)*tile_size_in_pixels;
            f32 min_x = center_x - 0.5f*tile_size_in_pixels;
            f32 min_y = center_y - 0.5f*tile_size_in_pixels;
            f32 max_x = center_x + 0.5f*tile_size_in_pixels;
            f32 max_y = center_y + 0.5f*tile_size_in_pixels;
            draw_rectangle(framebuffer, {min_x, min_y}, {max_x, max_y}, tile_color);
        }
    }
#endif
    
    for (u32 high_entity_index = 0;
         high_entity_index < state->high_entity_count;
         high_entity_index++) {
        high_entity_t *high_entity = state->high_entities_ + high_entity_index;
        low_entity_t *low_entity = state->low_entities + high_entity->low_entity_index;

        high_entity->pos += entity_offset_for_frame;
            
        f32 entity_ground_point_x = screen_center_x + meters_to_pixels*high_entity->pos.x;
        f32 entity_ground_point_y = screen_center_y - meters_to_pixels*high_entity->pos.y;

        vec2f entity_left_top = vec2f{entity_ground_point_x - 0.5f*meters_to_pixels*low_entity->width,
                                      entity_ground_point_y - 0.5f*meters_to_pixels*low_entity->height};
        vec2f entity_width_height = vec2f{low_entity->width, low_entity->height};
            
        vec3f entity_color = vec3f{1.0f, 1.0f, 0.0f};
        if (low_entity->type == ENTITY_TYPE_PLAYER) {
            draw_rectangle(framebuffer, entity_left_top, entity_left_top + meters_to_pixels*entity_width_height, entity_color);
                
            bitmap_t *player_sprite = &state->player_sprites[high_entity->direction];
            draw_bitmap(framebuffer, player_sprite, entity_ground_point_x - (tile_size_in_pixels/2), entity_ground_point_y - tile_size_in_pixels);
        } else {
            draw_rectangle(framebuffer, entity_left_top, entity_left_top + meters_to_pixels*entity_width_height, entity_color);
        }
    }
}
