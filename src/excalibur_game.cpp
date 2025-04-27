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
#include "excalibur_sim.h"
#include "excalibur_entity.h"

#include "excalibur_game.h"

internal low_entity_t *
get_low_entity(game_state_t *state, u32 index) {
    low_entity_t *result = 0;
    if ((index > 0) && (index < state->low_entity_count)) {
        result = state->low_entities + index;
    }
    return(result);
}

#include "excalibur_world.cpp"
#include "excalibur_sim.cpp"
#include "excalibur_entity.cpp"

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

struct add_low_entity_result {
    u32 low_index;
    low_entity_t *low;
};

internal add_low_entity_result
add_low_entity(game_state_t *state, entity_type_t type, world_position_t pos) {
    EX_ASSERT(state->low_entity_count < EX_ARRAY_COUNT(state->low_entities));
    u32 entity_index = state->low_entity_count++;
    
    low_entity_t *low_entity = state->low_entities + entity_index;
    *low_entity = {};
    low_entity->sim.type = type;
    low_entity->pos = null_position();

    change_entity_location(&state->world_arena, state->world, entity_index, low_entity, pos);
    
    add_low_entity_result result = {};
    result.low_index = entity_index;
    result.low = low_entity;
    
    return(result);
}

internal add_low_entity_result
add_wall(game_state_t *state, u32 tile_x, u32 tile_y, u32 tile_z) {
    world_position_t pos = chunk_position_from_tile_position(state->world, tile_x, tile_y, tile_z);
    add_low_entity_result entity = add_low_entity(state, ENTITY_TYPE_WALL, pos);
    
    entity.low->sim.height = state->world->tile_side_in_meters;
    entity.low->sim.width = entity.low->sim.height;
    add_entity_flag(&entity.low->sim, ENTITY_FLAG_COLLIDES);
    
    return(entity);
}

internal void
init_hit_points(low_entity_t *low_entity, u32 hit_point_count) {
    EX_ASSERT(hit_point_count < EX_ARRAY_COUNT(low_entity->sim.hit_points));
    low_entity->sim.hit_point_max = hit_point_count;
    for (u32 hit_point_index = 0;
         hit_point_index < hit_point_count;
         hit_point_index++) {
        hit_point_t *hit_point = low_entity->sim.hit_points + hit_point_index;
        hit_point->flags = 0;
        hit_point->filled_amount = HIT_POINT_FILLED_MAX;
    }
}

internal add_low_entity_result
add_sword(game_state_t *state) {
    add_low_entity_result entity = add_low_entity(state, ENTITY_TYPE_SWORD, null_position());
    
    entity.low->sim.height = state->world->tile_side_in_meters;
    entity.low->sim.width = entity.low->sim.height;
    
    return(entity);
}

internal add_low_entity_result
add_player(game_state_t *state) {
    world_position_t pos = state->camera_pos;
    add_low_entity_result entity = add_low_entity(state, ENTITY_TYPE_PLAYER, pos);
    
    // TODO(xkazu0x): colliding is not happening in perfect tile size,
    // theres is something sketchy on how we are representing the
    // colliding dimensions
    entity.low->sim.height = 0.9f*state->world->tile_side_in_meters;
    entity.low->sim.width = entity.low->sim.height;
    add_entity_flag(&entity.low->sim, ENTITY_FLAG_COLLIDES);
    
    init_hit_points(entity.low, 3);

    add_low_entity_result sword = add_sword(state);
    entity.low->sim.sword.index = sword.low_index;
    
    if (state->camera_following_entity_index == 0) {
        state->camera_following_entity_index = entity.low_index;
    }
    
    return(entity);
}

internal add_low_entity_result
add_familiar(game_state_t *state, u32 tile_x, u32 tile_y, u32 tile_z) {
    world_position_t pos = chunk_position_from_tile_position(state->world, tile_x, tile_y, tile_z);
    add_low_entity_result entity = add_low_entity(state, ENTITY_TYPE_FAMILIAR, pos);
    
    entity.low->sim.height = 0.5f*state->world->tile_side_in_meters;
    entity.low->sim.width = entity.low->sim.height;
    add_entity_flag(&entity.low->sim, ENTITY_FLAG_COLLIDES);

    return(entity);
}

internal add_low_entity_result
add_monster(game_state_t *state, u32 tile_x, u32 tile_y, u32 tile_z) {
    world_position_t pos = chunk_position_from_tile_position(state->world, tile_x, tile_y, tile_z);
    add_low_entity_result entity = add_low_entity(state, ENTITY_TYPE_MONSTER, pos);
    
    entity.low->sim.height = state->world->tile_side_in_meters;
    entity.low->sim.width = entity.low->sim.height;
    add_entity_flag(&entity.low->sim, ENTITY_FLAG_COLLIDES);
    
    init_hit_points(entity.low, 5);
    
    return(entity);
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

internal void
draw_hit_points(entity_visible_piece_group_t *piece_group, sim_entity_t *entity) {
    if (entity->hit_point_max >= 1) {
        vec2f health_dim = _vec2f(0.125f*1.4f);
        f32 spacing_x = 1.5f*health_dim.x;
        f32 offset_x = (entity->width + (spacing_x/2))/2;
        vec2f hit_pos = _vec2f((-0.5f*(entity->hit_point_max - 1)*spacing_x) + offset_x, 0.7f);
        vec2f hit_dpos = _vec2f(spacing_x, 0.0f);
                        
        for (u32 health_index = 0;
             health_index < entity->hit_point_max;
             health_index++) {
            hit_point_t *hit_point = entity->hit_points + health_index;
            vec4f color = _vec4f(222.0f/255.0f, 214.0f/255.0f, 222.0f/255.0f, 1.0f);
            if (hit_point->filled_amount == 0) {
                color = _vec4f(164.0f/255.0f, 157.0f/255.0f, 164.0f/255.0f, 1.0f);
            }
            push_rect(piece_group, _vec3f(hit_pos, 0.0f), health_dim, color, 0.0f);
            hit_pos += hit_dpos;
        }
    }
}

extern "C" GAME_UPDATE_AND_RENDER(game_update_and_render) {
    EX_ASSERT(sizeof(game_state_t) <= memory->permanent_storage_size);
    game_state_t *state = (game_state_t *)memory->permanent_storage;
    
    u32 tile_count_x = 17;
    u32 tile_count_y = 9;
    s32 tile_size_in_pixels = 16;
    
    if (!memory->initialized) {
        add_low_entity(state, ENTITY_TYPE_NULL, null_position());

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
        state->sword_sprite = debug_load_bitmap(memory->debug_os_read_file, thread, "res/floor.bmp");
        
        state->world_arena = memory_arena_create(memory->permanent_storage_size - sizeof(game_state_t),
                                                 (u8 *)memory->permanent_storage + sizeof(game_state_t));
        state->world = (world_t *)memory_arena_push(&state->world_arena, sizeof(world_t));
        world_t *world = state->world;
        world_initialize(world, 1.4f);

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
        state->camera_pos = new_camera_pos;
        
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
        
        memory->initialized = EX_TRUE;
    }

    world_t *world = state->world;
    f32 meters_to_pixels = state->meters_to_pixels;

    /////////////////////////////////
    // NOTE(xkazu0x): gamepad control
    for (u32 gamepad_index = 0; gamepad_index < EX_ARRAY_COUNT(input->gamepads); gamepad_index++) {
        gamepad_t *gamepad = get_gamepad(input, gamepad_index);
        controlled_player_t *controlled_player = state->controlled_players + gamepad_index;
        if (controlled_player->entity_index == 0) {
            if ((gamepad->start.pressed) ||
                ((gamepad_index == 1) && (input->keyboard[KEY_SPACE].pressed))) {
                *controlled_player = {};
                controlled_player->entity_index = add_player(state).low_index;
            }
        } else {
            controlled_player->dd_pos = {};
            controlled_player->d_z = 0.0f;
            controlled_player->d_sword = {};
            
            controlled_player->dd_pos = _vec2f(gamepad->left_stick.x, gamepad->left_stick.y);

            // NOTE(xkazu0x): combine keyboard and gamepad inputs
            // only for the first player
            if (gamepad_index == 1) {
                if (input->keyboard[KEY_W].down) controlled_player->dd_pos.y = 1.0f;
                if (input->keyboard[KEY_S].down) controlled_player->dd_pos.y = -1.0f;
                if (input->keyboard[KEY_A].down) controlled_player->dd_pos.x = -1.0f;
                if (input->keyboard[KEY_D].down) controlled_player->dd_pos.x = 1.0f;
                
                if (input->keyboard[KEY_UP].pressed) controlled_player->d_sword = _vec2f(0.0f, 1.0f);
                if (input->keyboard[KEY_DOWN].pressed) controlled_player->d_sword = _vec2f(0.0f, -1.0f);
                if (input->keyboard[KEY_LEFT].pressed) controlled_player->d_sword = _vec2f(-1.0f, 0.0f);
                if (input->keyboard[KEY_RIGHT].pressed) controlled_player->d_sword = _vec2f(1.0f, 0.0f);

                if (input->keyboard[KEY_SPACE].pressed) controlled_player->d_z = 3.0f;
            }
        }
    }

    //////////////////////////////////
    // NOTE(xkazu0x): simulation begin
    u32 tile_span_x = 17*3;
    u32 tile_span_y = 9*3;
    rect2f camera_bounds = rect2f_center_dim(_vec2f(0.0f), world->tile_side_in_meters*_vec2f((f32)tile_span_x, (f32)tile_span_y));
    
    memory_arena_t sim_arena = memory_arena_create(memory->transient_storage_size, memory->transient_storage);
    sim_region_t *sim_region = begin_sim(&sim_arena, state, world, state->camera_pos, camera_bounds);

    ////////////////////////
    // NOTE(xkazu0x): render
    for (u32 y = 0; y < 12; y++) {
        for (u32 x = 0; x < 20; x++) {
            vec3f color = _vec3f(57.0f/255.0f, 44.0f/255.0f, 49.0f/255.0f);
            
            if (((y % 2 == 0) && (x % 2 == 0)) ||
                ((y % 2 == 1) && (x % 2 == 1))) {
                color = _vec3f(74.0f/255.0f, 60.0f/255.0f, 74.0f/255.0f);
            }
            
            vec2f min = _vec2f(x*tile_size_in_pixels, y*tile_size_in_pixels);
            vec2f max = _vec2f(min.x + tile_size_in_pixels, min.y + tile_size_in_pixels);
            
            draw_rectangle(framebuffer, min, max, color);
        }
    }
    
    f32 screen_center_x = 0.5f*((f32)framebuffer->width);
    f32 screen_center_y = 0.5f*((f32)framebuffer->height);

    entity_visible_piece_group_t piece_group;
    piece_group.game_state = state;

    sim_entity_t *entity = sim_region->entities;
    for (u32 entity_index = 0;
         entity_index < sim_region->entity_count;
         ++entity_index, ++entity) {
        piece_group.piece_count = 0;
        f32 delta = clock->delta_seconds;

        f32 shadow_alpha = 1.0f - 0.5f*entity->z;
        if (shadow_alpha < 0.0f) shadow_alpha = 0.0f;
        
        switch (entity->type) {
            case ENTITY_TYPE_PLAYER: {
                for (u32 controlled_index = 0;
                     controlled_index < EX_ARRAY_COUNT(state->controlled_players);
                     ++controlled_index) {
                    controlled_player_t *controlled_player = state->controlled_players + controlled_index;
                    if (entity->storage_index == controlled_player->entity_index) {
                        if (controlled_player->d_z != 0.0f) {
                            entity->d_z = controlled_player->d_z;
                        }
                        move_spec_t move_spec = default_move_spec();
                        move_spec.unit_max_accel_vector = EX_TRUE;
                        move_spec.speed = 50.0f;
                        move_spec.drag = 5.0f;
                        move_entity(sim_region, entity, clock->delta_seconds, &move_spec, controlled_player->dd_pos);
                        if ((controlled_player->d_sword.x != 0.0f) || (controlled_player->d_sword.y != 0.0f)) {
                            sim_entity_t *sword = entity->sword.ptr;
                            if (sword && is_entity_flag_set(sword, ENTITY_FLAG_NON_SPATIAL)) {
                                sword->distance_remaining = 3.0f;
                                make_entity_spatial(sword, entity->pos, 5.0f*controlled_player->d_sword);
                            }
                        }
                    }
                }

                bitmap_t *sprite = &state->player_sprites[entity->direction];
                push_bitmap(&piece_group, &state->shadow_sprite, _vec3f(0.0f), shadow_alpha, 0.0f);
                push_bitmap(&piece_group, sprite, _vec3f(0.0f, 0.0f, -0.4f));
                draw_hit_points(&piece_group, entity);
            } break;
            case ENTITY_TYPE_WALL: {
                push_bitmap(&piece_group, &state->wall_sprite, _vec3f(0.0f));
            } break;
            case ENTITY_TYPE_SWORD: {
                update_sword(sim_region, entity, delta);
                push_bitmap(&piece_group, &state->shadow_sprite, _vec3f(0.0f));
            } break;
            case ENTITY_TYPE_FAMILIAR: {
                update_familiar(sim_region, entity, delta);
                
                entity->t_bob += delta;
                if (entity->t_bob > (2.0f*pi32)) {
                    entity->t_bob -= (2.0f*pi32);
                }

                f32 bob_sin = sin_f32(5.0f*entity->t_bob);
                
                push_bitmap(&piece_group, &state->shadow_sprite, _vec3f(0.0f), (0.5f*shadow_alpha) + 0.2f*bob_sin, 0.0f);
                push_bitmap(&piece_group, &state->bat_sprite, _vec3f(0.0f, 0.0f, 0.2f*bob_sin - 0.6f));
            } break;
            case ENTITY_TYPE_MONSTER: {
                update_monster(sim_region, entity, delta);
                push_bitmap(&piece_group, &state->shadow_sprite, _vec3f(0.0f), shadow_alpha, 0.0f);
                push_bitmap(&piece_group, &state->player_sprites[2], _vec3f(0.0f, 0.0f, -0.4f));
                draw_hit_points(&piece_group, entity);
            } break;
            default: {
                INVALID_CODE_PATH();
            }
        }

        f32 dd_z = -9.8f;
        entity->z = 0.5f*dd_z*sqr(delta) + entity->d_z*delta + entity->z;
        entity->d_z = dd_z*delta + entity->d_z;
        if (entity->z < 0.0f) entity->z = 0.0f;
        
        f32 entity_ground_point_x = screen_center_x + meters_to_pixels*entity->pos.x;
        f32 entity_ground_point_y = screen_center_y - meters_to_pixels*entity->pos.y;
        f32 entity_z = -meters_to_pixels*entity->z;
        
#if 0
        vec2f entity_left_top = _vec2f(entity_ground_point_x - 0.5f*meters_to_pixels*low_entity->sim.width,
                                       entity_ground_point_y - 0.5f*meters_to_pixels*low_entity->sim.height);
        vec2f entity_width_height = _vec2f(low_entity->sim.width, low_entity->sim.height);
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

    ////////////////////////////////
    // NOTE(xkazu0x): simulation end
    world_position_t world_origin = {};
    vec3f delta_origin = subtract(sim_region->world, &world_origin, &sim_region->origin);
    draw_rectangle(framebuffer, _vec2f(delta_origin.x, delta_origin.y), _vec2f(tile_size_in_pixels), _vec3f(1.0f));
    
    end_sim(sim_region, state);
}
