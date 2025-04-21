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
    vec2i mini = _vec2i(0, 0);
    vec2i maxi = _vec2i(0, 0);
    
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
draw_bitmap(os_framebuffer_t *framebuffer, bitmap_t *bitmap,
            f32 x_offset, f32 y_offset, f32 c_alpha = 1.0f) {
    s32 x_min = round_f32_to_s32(x_offset);
    s32 y_min = round_f32_to_s32(y_offset);
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

inline vec2f
get_camera_space_pos(game_state_t *state, low_entity_t *low_entity) {
    // NOTE(xkazu0x): map the entity in camera space
    vec3f diff = subtract(state->world, &low_entity->pos, &state->camera_pos);
    vec2f result = _vec2f(diff.x, diff.y);
    return(result);
}

inline high_entity_t *
make_entity_high(game_state_t *state, low_entity_t *low_entity, u32 low_index, vec2f camera_space_pos) {
    high_entity_t *high_entity = 0;
    
    EX_ASSERT(low_entity->high_entity_index == 0);
    if (low_entity->high_entity_index == 0) {
        if (state->high_entity_count < EX_ARRAY_COUNT(state->high_entities_)) {
            u32 high_index = state->high_entity_count++;
            high_entity = state->high_entities_ + high_index;

            high_entity->pos = camera_space_pos;
            high_entity->d_pos = _vec2f(0.0f);
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

internal high_entity_t *
make_entity_high(game_state_t *state, u32 low_index) {
    high_entity_t *high_entity = 0;
    
    low_entity_t *low_entity = state->low_entities + low_index;
    if (low_entity->high_entity_index) {
        high_entity = state->high_entities_ + low_entity->high_entity_index;
    } else {
        vec2f camera_space_pos = get_camera_space_pos(state, low_entity);
        high_entity = make_entity_high(state, low_entity, low_index, camera_space_pos);
    }

    return(high_entity);
}

internal entity_t
force_entity_into_high(game_state_t *state, u32 low_index) {
    entity_t result = {};
    if ((low_index > 0) && (low_index < state->low_entity_count)) {
        result.low_index = low_index;
        result.low = state->low_entities + low_index;
        result.high = make_entity_high(state, low_index);
    }
    return(result);
}

internal void
make_entity_low(game_state_t *state, u32 low_index) {
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

internal low_entity_t *
get_low_entity(game_state_t *state, u32 index) {
    low_entity_t *result = 0;
    if ((index > 0) && (index < state->low_entity_count)) {
        result = state->low_entities + index;
    }
    return(result);
}

struct add_low_entity_result {
    u32 low_index;
    low_entity_t *low;
};

internal add_low_entity_result
add_low_entity(game_state_t *state, entity_type_t type, world_position_t *pos) {
    EX_ASSERT(state->low_entity_count < EX_ARRAY_COUNT(state->low_entities));
    u32 entity_index = state->low_entity_count++;
    
    low_entity_t *low_entity = state->low_entities + entity_index;
    *low_entity = {};
    low_entity->type = type;

    if (pos) {
        low_entity->pos = *pos;
        entity_change_location(&state->world_arena, state->world, entity_index, 0, pos);
    }
    
    add_low_entity_result result = {};
    result.low_index = entity_index;
    result.low = low_entity;
    
    return(result);
}

internal add_low_entity_result
add_wall(game_state_t *state, u32 tile_x, u32 tile_y, u32 tile_z) {
    world_position_t pos = chunk_position_from_tile_position(state->world, tile_x, tile_y, tile_z);
    add_low_entity_result entity = add_low_entity(state, ENTITY_TYPE_WALL, &pos);
    
    entity.low->height = state->world->tile_side_in_meters;
    entity.low->width = entity.low->height;
    entity.low->collides = EX_TRUE;
    
    return(entity);
}

internal add_low_entity_result
add_player(game_state_t *state) {
    world_position_t pos = state->camera_pos;
    add_low_entity_result entity = add_low_entity(state, ENTITY_TYPE_PLAYER, &pos);

    entity.low->hit_point_max = 3;
    entity.low->hit_points[2].filled_amount = HIT_POINT_FILLED_MAX;
    entity.low->hit_points[0] = entity.low->hit_points[1] = entity.low->hit_points[2];
    
    // TODO(xkazu0x): colliding is not happening in perfect tile size
    entity.low->height = 0.9f*state->world->tile_side_in_meters;
    entity.low->width = entity.low->height;
    entity.low->collides = EX_TRUE;
    
    if (state->camera_following_entity_index == 0) {
        state->camera_following_entity_index = entity.low_index;
    }
    
    return(entity);
}

internal add_low_entity_result
add_monster(game_state_t *state, u32 tile_x, u32 tile_y, u32 tile_z) {
    world_position_t pos = chunk_position_from_tile_position(state->world, tile_x, tile_y, tile_z);
    add_low_entity_result entity = add_low_entity(state, ENTITY_TYPE_MONSTER, &pos);
    
    entity.low->height = state->world->tile_side_in_meters;
    entity.low->width = entity.low->height;
    entity.low->collides = EX_TRUE;

    return(entity);
}

internal add_low_entity_result
add_familiar(game_state_t *state, u32 tile_x, u32 tile_y, u32 tile_z) {
    world_position_t pos = chunk_position_from_tile_position(state->world, tile_x, tile_y, tile_z);
    add_low_entity_result entity = add_low_entity(state, ENTITY_TYPE_FAMILIAR, &pos);
    
    entity.low->height = 0.5f*state->world->tile_side_in_meters;
    entity.low->width = entity.low->height;
    entity.low->collides = EX_TRUE;

    return(entity);
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
move_entity(game_state_t *state, entity_t entity, vec2f dd_pos, f32 delta) {
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
            
                    vec2f min_corner = -0.5f*_vec2f(dim_w, dim_h);
                    vec2f max_corner =  0.5f*_vec2f(dim_w, dim_h);
                
                    vec2f rel = entity.high->pos - test_entity.high->pos;

                    if (wall_test(min_corner.x, rel.x, rel.y, player_delta.x, player_delta.y,
                                  &t_min, min_corner.y, max_corner.y)) {
                        wall_normal = _vec2f(-1.0f, 0.0f);
                        hit_high_entity_index = test_high_entity_index;
                    }
                
                    if (wall_test(max_corner.x, rel.x, rel.y, player_delta.x, player_delta.y,
                                  &t_min, min_corner.y, max_corner.y)) {
                        wall_normal = _vec2f(1.0f, 0.0f);
                        hit_high_entity_index = test_high_entity_index;
                    }
                
                    if (wall_test(min_corner.y, rel.y, rel.x, player_delta.y, player_delta.x,
                                  &t_min, min_corner.x, max_corner.x)) {
                        wall_normal = _vec2f(0.0f, -1.0f);
                        hit_high_entity_index = test_high_entity_index;
                    }
                
                    if (wall_test(max_corner.y, rel.y, rel.x, player_delta.y, player_delta.x,
                                  &t_min, min_corner.x, max_corner.x)) {
                        wall_normal = _vec2f(0.0f, 1.0f);
                        hit_high_entity_index = test_high_entity_index;
                    }
                }
            }
        }
        
        entity.high->pos += t_min*player_delta;
        if (hit_high_entity_index) {
            entity.high->d_pos = entity.high->d_pos - 1*vec_dot(entity.high->d_pos, wall_normal)*wall_normal;
            player_delta = desired_pos - entity.high->pos;
            player_delta = player_delta - 1*vec_dot(player_delta, wall_normal)*wall_normal;

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

    world_position_t new_pos = map_into_chunk_space(state->world, state->camera_pos, entity.high->pos);
    entity_change_location(&state->world_arena, state->world,
                           entity.low_index, &entity.low->pos, &new_pos);
    entity.low->pos = new_pos;
}

inline b32
validate_entity_pairs(game_state_t *state) {
    b32 valid = EX_TRUE;
    
    for (u32 high_entity_index = 1;
         high_entity_index < state->high_entity_count;
         ++high_entity_index) {
        high_entity_t *high = state->high_entities_ + high_entity_index;
        valid = valid && (state->low_entities[high->low_entity_index].high_entity_index == high_entity_index);
    }

    return(valid);
}

internal void
offset_and_check_frequency_by_area(game_state_t *state,
                                   vec2f offset,
                                   rect2f high_frequency_bounds) {
    for (u32 high_entity_index = 1;
         high_entity_index < state->high_entity_count;
         ) {
        high_entity_t *high = state->high_entities_ + high_entity_index;
        high->pos += offset;
        
        if (is_in_rect(high_frequency_bounds, high->pos)) {
            ++high_entity_index;
        } else {
            EX_ASSERT(state->low_entities[high->low_entity_index].high_entity_index == high_entity_index);
            make_entity_low(state, high->low_entity_index);
        }
    }
}

internal void
set_camera(game_state_t *state, world_position_t new_camera_pos) {
    world_t *world = state->world;

    EX_ASSERT(validate_entity_pairs(state));
    
    vec3f camera_dpos = subtract(world, &new_camera_pos, &state->camera_pos);
    state->camera_pos = new_camera_pos;

    u32 tile_span_x = 17*3;
    u32 tile_span_y = 9*3;
    rect2f camera_bounds = rect2f_center_dim(_vec2f(0.0f),
                                             world->tile_side_in_meters*_vec2f((f32)tile_span_x,
                                                                                     (f32)tile_span_y));
    vec2f entity_offset_for_frame = -_vec2f(camera_dpos.x, camera_dpos.y);
    offset_and_check_frequency_by_area(state, entity_offset_for_frame, camera_bounds);
    
    EX_ASSERT(validate_entity_pairs(state));
    
    world_position_t min_chunk_pos = map_into_chunk_space(state->world, new_camera_pos, rect_get_min_corner(camera_bounds));
    world_position_t max_chunk_pos = map_into_chunk_space(state->world, new_camera_pos, rect_get_max_corner(camera_bounds));
    for (s32 chunk_y = min_chunk_pos.chunk_y;
         chunk_y <= max_chunk_pos.chunk_y;
         ++chunk_y) {
        for (s32 chunk_x = min_chunk_pos.chunk_x;
             chunk_x <= max_chunk_pos.chunk_x;
             ++chunk_x) {
            world_chunk_t *chunk = get_world_chunk(state->world, chunk_x, chunk_y, new_camera_pos.chunk_z);
            if (chunk) {
                for (world_entity_block_t *block = &chunk->entity_block;
                     block;
                     block = block->next) {
                    for (u32 entity_index_index = 0;
                         entity_index_index < block->entity_count;
                         entity_index_index++) {
                        u32 low_entity_index = block->low_entity_index[entity_index_index];
                        low_entity_t *low = state->low_entities + low_entity_index;
                        if (low->high_entity_index == 0) {
                            vec2f camera_space_pos = get_camera_space_pos(state, low);
                            if (is_in_rect(camera_bounds, camera_space_pos)) {
                                make_entity_high(state, low, low_entity_index, camera_space_pos);
                            }
                        }
                    }
                }
            }
        }
    }
    
    EX_ASSERT(validate_entity_pairs(state));
}

internal void
push_piece(entity_visible_piece_group_t *group, bitmap_t *bitmap,
           vec3f offset, vec2f size, vec4f color, f32 entity_zc = 1.0f) {

    EX_ASSERT(group->piece_count < EX_ARRAY_COUNT(group->pieces));
    entity_visible_piece_t *piece = group->pieces + group->piece_count++;
    piece->bitmap = bitmap;
    piece->offset = group->game_state->meters_to_pixels*_vec3f(offset.x, -offset.y, offset.z);
    piece->color = color;
    piece->size = size;
    piece->entity_zc = entity_zc;
}

internal void
push_bitmap(entity_visible_piece_group_t *group, bitmap_t *bitmap, vec3f offset,
            f32 alpha = 1.0f, f32 entity_zc = 1.0f) {
    push_piece(group, bitmap, offset, _vec2f(0.0f), _vec4f(_vec3f(1.0f), alpha), entity_zc);
}

internal void
push_rect(entity_visible_piece_group_t *group, vec3f offset, vec2f size, vec4f color,
          f32 entity_zc = 1.0f) {
    push_piece(group, 0, offset, size, color, entity_zc);
}

internal entity_t
entity_from_high_index(game_state_t *state, u32 index) {
    entity_t result = {};
    
    if (index) {
        EX_ASSERT(index < EX_ARRAY_COUNT(state->high_entities_));
        result.high = state->high_entities_ + index;
        result.low_index = result.high->low_entity_index;
        result.low = state->low_entities + result.low_index;
    }

    return(result);
}

internal void
update_familiar(game_state_t *state, entity_t entity, f32 delta) {
    entity_t closest_player = {};
    f32 closest_player_distance_sqr = sqr(10.0f); // NOTE(xkazu0x): ten meter maximun search!
    
    for (u32 high_entity_index = 1;
         high_entity_index < state->high_entity_count;
         high_entity_index++) {
        entity_t test_entity = entity_from_high_index(state, high_entity_index);
        if (test_entity.low->type == ENTITY_TYPE_PLAYER) {
            f32 test_distance_sqr = length_sqr(test_entity.high->pos - entity.high->pos);
            if (closest_player_distance_sqr > test_distance_sqr) {
                closest_player = test_entity;
                closest_player_distance_sqr = test_distance_sqr;
            }
        }
    }

    vec2f dd_pos = _vec2f(0.0f);
    if (closest_player.high && (closest_player_distance_sqr > sqr(2.5f))) {
        // TODO(xkazu0x): pull speed out of move entity
        f32 acceleration = 0.5f;
        f32 lenght_normalized = acceleration / sqrt_f32(closest_player_distance_sqr);
        dd_pos = lenght_normalized*(closest_player.high->pos - entity.high->pos);
    }

    move_entity(state, entity, dd_pos, delta);
    if (dd_pos.x == 0.0f && dd_pos.y == 0.0f) {
        entity.high->direction = 2;
    }
}

internal void
update_monster(game_state_t *state, entity_t entity, f32 delta) {

}

extern "C" GAME_UPDATE_AND_RENDER(game_update_and_render) {
    EX_ASSERT(sizeof(game_state_t) <= memory->permanent_storage_size);
    game_state_t *state = (game_state_t *)memory->permanent_storage;
    
    u32 tile_count_x = 17;
    u32 tile_count_y = 9;
    
    if (!memory->initialized) {
        add_low_entity(state, ENTITY_TYPE_NULL, 0);
        state->high_entity_count = 1;

        state->player_sprites[0] =
            debug_load_bitmap(memory->debug_os_read_file, thread, "res/skull_back.bmp");
        state->player_sprites[1] =
            debug_load_bitmap(memory->debug_os_read_file, thread, "res/skull_right.bmp");
        state->player_sprites[2] =
            debug_load_bitmap(memory->debug_os_read_file, thread, "res/skull_front.bmp");
        state->player_sprites[3] =
            debug_load_bitmap(memory->debug_os_read_file, thread, "res/skull_left.bmp");
        state->wall_sprite = debug_load_bitmap(memory->debug_os_read_file, thread, "res/wall.bmp");
        state->bat_sprite = debug_load_bitmap(memory->debug_os_read_file, thread, "res/bat.bmp");
        state->shadow_sprite = debug_load_bitmap(memory->debug_os_read_file, thread, "res/shadow.bmp");
        
        state->world_arena = memory_arena_create(memory->permanent_storage_size - sizeof(game_state_t),
                                               (u8 *)memory->permanent_storage + sizeof(game_state_t));
        state->world = (world_t *)memory_arena_push(&state->world_arena, sizeof(world_t));
        world_t *world = state->world;
        world_initialize(world, 1.4f);

        s32 tile_size_in_pixels = 16;
        state->meters_to_pixels = (f32)tile_size_in_pixels / (f32)world->tile_side_in_meters;
        
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
                        
                    if (tile_value == 2) {
                        add_wall(state, abs_tile_x, abs_tile_y, abs_tile_z);
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

        u32 camera_tile_x = screen_base_x*tile_count_x + tile_count_x/2;
        u32 camera_tile_y = screen_base_y*tile_count_y + tile_count_y/2;
        u32 camera_tile_z = screen_base_z;
        
        world_position_t new_camera_pos = {};
        new_camera_pos = chunk_position_from_tile_position(state->world, camera_tile_x, camera_tile_y, camera_tile_z);

        add_monster(state, camera_tile_x + 2, camera_tile_y + 2, camera_tile_z);
        for (u32 familiar_index = 0;
             familiar_index < 1;
             ++familiar_index) {
            s32 familiar_offset_x = (random_number_table[random_number_index++] % 10) - 5;
            s32 familiar_offset_y = (random_number_table[random_number_index++] % 10) - 3;
            if ((familiar_offset_x != 0) ||
                (familiar_offset_y != 0)) {
                add_familiar(state, camera_tile_x + familiar_offset_x, camera_tile_y + familiar_offset_y, camera_tile_z);
            }
        }
        
        set_camera(state, new_camera_pos);        
        memory->initialized = EX_TRUE;
    }

    world_t *world = state->world;
    f32 meters_to_pixels = state->meters_to_pixels;

    ////////////////////////////////
    // NOTE(xkazu0x): player control
    for (u32 gamepad_index = 0; gamepad_index < EX_ARRAY_COUNT(input->gamepads); gamepad_index++) {
        gamepad_t *gamepad = get_gamepad(input, gamepad_index);
        u32 low_entity_index = state->player_gamepad_index[gamepad_index];
        if (low_entity_index == 0) {
            if (gamepad->start.pressed) {
                u32 player_entity_index = add_player(state).low_index;
                state->player_gamepad_index[gamepad_index] = player_entity_index;
            }
            
            // NOTE(xkazu0x): combine keyboard and gamepad inputs
            // only for the first player
            if (gamepad_index == 1) {
                if (input->keyboard[KEY_SPACE].pressed) {
                    u32 player_entity_index = add_player(state).low_index;
                    state->player_gamepad_index[gamepad_index] = player_entity_index;
                }
            }
        } else {
            entity_t controlling_entity = force_entity_into_high(state, low_entity_index);
            
            vec2f dd_pos = {};
            dd_pos = _vec2f(gamepad->left_stick.x, gamepad->left_stick.y);

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
            
            move_entity(state, controlling_entity, dd_pos, clock->delta_seconds);
        }
    }
    
    ///////////////////////////////
    // NOTE(xkazu0x): camera update
    entity_t camera_following_entity = force_entity_into_high(state, state->camera_following_entity_index);
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
        
        set_camera(state, new_camera_pos);
    }
    
    ////////////////////////
    // NOTE(xkazu0x): render
    for (u32 y = 0; y < 12; y++) {
        for (u32 x = 0; x < 20; x++) {
            vec3f color = _vec3f(57.0f/255.0f, 44.0f/255.0f, 49.0f/255.0f);
            
            if (((y % 2 == 0) && (x % 2 == 0)) ||
                ((y % 2 == 1) && (x % 2 == 1))) {
                color = _vec3f(74.0f/255.0f, 60.0f/255.0f, 74.0f/255.0f);
            }
            
            vec2f min = _vec2f(x*16, y*16);
            vec2f max = _vec2f(min.x + 16, min.y + 16);
            
            draw_rectangle(framebuffer, min, max, color);
        }
    }
    
    f32 screen_center_x = 0.5f*((f32)framebuffer->width);
    f32 screen_center_y = 0.5f*((f32)framebuffer->height);

    entity_visible_piece_group_t piece_group;
    piece_group.game_state = state;
    for (u32 high_entity_index = 1;
         high_entity_index < state->high_entity_count;
         high_entity_index++) {
        piece_group.piece_count = 0;
        
        high_entity_t *high_entity = state->high_entities_ + high_entity_index;
        low_entity_t *low_entity = state->low_entities + high_entity->low_entity_index;

        entity_t entity;
        entity.low_index = high_entity->low_entity_index;
        entity.low = low_entity;
        entity.high = high_entity;
        
        f32 delta = clock->delta_seconds;

        f32 shadow_alpha = 1.0f - 0.5f*high_entity->z;
        if (shadow_alpha < 0.0f) shadow_alpha = 0.0f;
        
        switch (low_entity->type) {
            case ENTITY_TYPE_PLAYER: {
                bitmap_t *sprite = &state->player_sprites[high_entity->direction];
                push_bitmap(&piece_group, &state->shadow_sprite, _vec3f(0.0f), shadow_alpha, 0.0f);
                push_bitmap(&piece_group, sprite, _vec3f(0.0f, 0.0f, -0.4f));

                if (low_entity->hit_point_max >= 1) {
                    vec2f health_dim = _vec2f(0.125f*1.4f);
                    f32 spacing_x = 1.5f*health_dim.x;
                    f32 offset_x = health_dim.x*low_entity->hit_point_max + spacing_x/2;
                    vec2f hit_pos = _vec2f((-0.5f*(low_entity->hit_point_max - 1)*spacing_x) + offset_x, 0.7f);
                    vec2f hit_dpos = _vec2f(spacing_x, 0.0f);
                        
                    for (u32 health_index = 0;
                         health_index < low_entity->hit_point_max;
                         health_index++) {
                        hit_point_t *hit_point = low_entity->hit_points + health_index;
                        vec4f color = _vec4f(222.0f/255.0f, 214.0f/255.0f, 222.0f/255.0f, 1.0f);
                        if (hit_point->filled_amount == 0) {
                            color = _vec4f(164.0f/255.0f, 157.0f/255.0f, 164.0f/255.0f, 1.0f);
                        }
                        push_rect(&piece_group, _vec3f(hit_pos, 0.0f), health_dim, color, 0.0f);
                        hit_pos += hit_dpos;
                    }
                }
            } break;
            case ENTITY_TYPE_WALL: {
                push_bitmap(&piece_group, &state->wall_sprite, _vec3f(0.0f));
            } break;
            case ENTITY_TYPE_FAMILIAR: {
                update_familiar(state, entity, delta);
                
                entity.high->t_bob += delta;
                if (entity.high->t_bob > (2.0f*pi32)) {
                    entity.high->t_bob -= (2.0f*pi32);
                }

                f32 bob_sin = sin_f32(5.0f*entity.high->t_bob);
                
                push_bitmap(&piece_group, &state->shadow_sprite, _vec3f(0.0f), (0.5f*shadow_alpha) + 0.2f*bob_sin, 0.0f);
                push_bitmap(&piece_group, &state->bat_sprite, _vec3f(0.0f, 0.6f, 0.2f*bob_sin));
            } break;
            case ENTITY_TYPE_MONSTER: {
                update_monster(state, entity, delta);
                push_bitmap(&piece_group, &state->player_sprites[2], _vec3f(0.0f));
            } break;
            default: {
                INVALID_CODE_PATH();
            }
        }
        
        f32 entity_ground_point_x = screen_center_x + meters_to_pixels*high_entity->pos.x;
        f32 entity_ground_point_y = screen_center_y - meters_to_pixels*high_entity->pos.y;
        f32 entity_z = -meters_to_pixels*high_entity->z;
        
#if 0
        vec2f entity_left_top = _vec2f(entity_ground_point_x - 0.5f*meters_to_pixels*low_entity->width,
                                       entity_ground_point_y - 0.5f*meters_to_pixels*low_entity->height);
        vec2f entity_width_height = _vec2f(low_entity->width, low_entity->height);
        draw_rectangle(framebuffer,
                       entity_left_top,
                       entity_left_top + meters_to_pixels*entity_width_height,
                       _vec3f(0.0f, 0.0f, 1.0f));
#endif

        for (u32 piece_index = 0; piece_index < piece_group.piece_count; piece_index++) {
            entity_visible_piece_t *piece = piece_group.pieces + piece_index;
            vec2f center = _vec2f(entity_ground_point_x + piece->offset.x,
                                  entity_ground_point_y + piece->offset.y + piece->offset.z + piece->entity_zc*entity_z);
            if (piece->bitmap) {
                draw_bitmap(framebuffer, piece->bitmap, center.x, center.y, piece->color.a);
            } else {
                vec2f half_size = 0.5f*meters_to_pixels*piece->size;
                vec3f color = _vec3f(piece->color.r, piece->color.g, piece->color.b);
                draw_rectangle(framebuffer, center - half_size, center + half_size, color);
            }
        }
    }
}
