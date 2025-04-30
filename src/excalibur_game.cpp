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
    ASSERT(index < GAMEPAD_MAX);
    gamepad_t *result = 0;
    if (index < GAMEPAD_MAX) {
        result = &input->gamepads[index];
    }
    return(result);
}

internal void
draw_rect(os_framebuffer_t *framebuffer, vec2 min, vec2 max, vec3 color) {
    s32 min_x = round_f32_to_s32(min.x);
    s32 min_y = round_f32_to_s32(min.y);
    
    s32 max_x = round_f32_to_s32(max.x);
    s32 max_y = round_f32_to_s32(max.y);

    if (min_x < 0) min_x = 0;
    if (min_y < 0) min_y = 0;
    
    if (max_x > framebuffer->width) max_x = framebuffer->width;
    if (max_y > framebuffer->height) max_y = framebuffer->height;

    u32 out_color = ((round_f32_to_u32(color.r * 255.0f) << 16) |
                     (round_f32_to_u32(color.g * 255.0f) << 8) |
                     (round_f32_to_u32(color.b * 255.0f) << 0));
    
    u8 *row = ((u8 *)framebuffer->pixels +
                 (min_x * framebuffer->bytes_per_pixel) +
                 (min_y * framebuffer->pitch));
    for (s32 y = min_y; y < max_y; y++) {
        u32 *pixel = (u32 *)row;
        for (s32 x = min_x; x < max_x; ++x) {
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

        ASSERT(header->compression == 3);
        
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

        ASSERT(red_shift.found);
        ASSERT(green_shift.found);
        ASSERT(blue_shift.found);
        ASSERT(alpha_shift.found);
        
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

inline vec2
get_camera_space_pos(game_state_t *state, low_entity_t *low_entity) {
    // NOTE(xkazu0x): map the entity in camera space
    vec3 diff = subtract(state->world, &low_entity->pos, &state->camera_pos);
    vec2 result = make_vec2(diff.x, diff.y);
    return(result);
}

struct add_low_entity_result {
    u32 low_index;
    low_entity_t *low;
};

internal add_low_entity_result
add_low_entity(game_state_t *state, entity_type_t type, world_position_t pos) {
    ASSERT(state->low_entity_count < ARRAY_COUNT(state->low_entities));
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
    ASSERT(hit_point_count < ARRAY_COUNT(low_entity->sim.hit_points));
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
           vec3 offset, vec2 size, vec4 color, f32 entity_zc = 1.0f) {

    ASSERT(group->piece_count < ARRAY_COUNT(group->pieces));
    entity_visible_piece_t *piece = group->pieces + group->piece_count++;
    piece->bitmap = bitmap;
    piece->offset = group->game_state->meters_to_pixels*make_vec3(offset.x, -offset.y, offset.z);
    piece->color = color;
    piece->size = size;
    piece->entity_zc = entity_zc;
}

internal void
push_bitmap(entity_visible_piece_group_t *group, bitmap_t *bitmap, vec3 offset,
            f32 alpha = 1.0f, f32 entity_zc = 1.0f) {
    push_piece(group, bitmap, offset, make_vec2(0.0f), make_vec4(make_vec3(1.0f), alpha), entity_zc);
}

internal void
push_rect(entity_visible_piece_group_t *group, vec3 offset, vec2 size, vec4 color,
          f32 entity_zc = 1.0f) {
    push_piece(group, 0, offset, size, color, entity_zc);
}

internal void
draw_hit_points(entity_visible_piece_group_t *piece_group, sim_entity_t *entity) {
    if (entity->hit_point_max >= 1) {
        vec2 health_dim = make_vec2(0.125f*1.4f);
        f32 spacing_x = 1.5f*health_dim.x;
        f32 offset_x = (entity->width + (spacing_x/2))/2;
        vec2 hit_pos = make_vec2((-0.5f*(entity->hit_point_max - 1)*spacing_x) + offset_x, 0.7f);
        vec2 hit_dpos = make_vec2(spacing_x, 0.0f);
                        
        for (u32 health_index = 0;
             health_index < entity->hit_point_max;
             health_index++) {
            hit_point_t *hit_point = entity->hit_points + health_index;
            vec4 color = make_vec4(222.0f/255.0f, 214.0f/255.0f, 222.0f/255.0f, 1.0f);
            if (hit_point->filled_amount == 0) {
                color = make_vec4(164.0f/255.0f, 157.0f/255.0f, 164.0f/255.0f, 1.0f);
            }
            push_rect(piece_group, make_vec3(hit_pos, 0.0f), health_dim, color, 0.0f);
            hit_pos += hit_dpos;
        }
    }
}

internal void
clear_collision_rules_for(game_state_t *state, u32 storage_index) {
    // TODO(xkazu0x): need to make a better data structure that allows
    // removal of collision rules without searching the entire table
    for (u32 hash_bucket = 0;
         hash_bucket < ARRAY_COUNT(state->collision_rule_hash);
         ++hash_bucket) {
        for (pairwise_collision_rule_t **rule = &state->collision_rule_hash[hash_bucket];
             *rule;
             ) {
            if (((*rule)->storage_index_a == storage_index) ||
                ((*rule)->storage_index_b == storage_index)) {
                pairwise_collision_rule_t *removed_rule = *rule;
                *rule = (*rule)->next_in_hash;
                
                removed_rule->next_in_hash = state->first_free_collision_rule;
                state->first_free_collision_rule = removed_rule;
            } else {
                rule = &(*rule)->next_in_hash;
            }
        }
    }
}

internal void
add_collision_rule(game_state_t *state, u32 storage_index_a, u32 storage_index_b, b32 should_collide) {
    // TODO(xkazu0x): collapse this with should_collide
    if (storage_index_a > storage_index_b) {
        u32 temp = storage_index_a;
        storage_index_a = storage_index_b;
        storage_index_b = temp;
    }

    // TODO(xkazu0x): BETTER HASH FUCTION
    pairwise_collision_rule_t *found = 0;
    u32 hash_bucket = storage_index_a & (ARRAY_COUNT(state->collision_rule_hash) - 1);
    for (pairwise_collision_rule_t *rule = state->collision_rule_hash[hash_bucket];
         rule;
         rule = rule->next_in_hash)
    {
        if ((rule->storage_index_a == storage_index_a) &&
            (rule->storage_index_b == storage_index_b)) {
            found = rule;
            break;
        }
    }

    if (!found) {
        found = state->first_free_collision_rule;
        if (found) {
            state->first_free_collision_rule = found->next_in_hash;
        } else {
            found = mema_push_struct(&state->world_arena, pairwise_collision_rule_t);
        }
        
        found->next_in_hash = state->collision_rule_hash[hash_bucket];
        state->collision_rule_hash[hash_bucket] = found;
    }

    if (found) {
        found->storage_index_a = storage_index_a;
        found->storage_index_b = storage_index_b;
        found->should_collide = should_collide;
    }
}

extern "C" GAME_UPDATE_AND_RENDER(game_update_and_render) {
    ASSERT(sizeof(game_state_t) <= memory->permanent_storage_size);
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
        b32 door_left = false;
        b32 door_right = false;
        b32 door_top = false;
        b32 door_bottom = false;
        b32 door_up = false;
        b32 door_down = false;        
        for (u32 screen_index = 0; screen_index < 10; screen_index++) {
            // TODO(xkazu0x): random number generator
            ASSERT(random_number_index < ARRAY_COUNT(random_number_table));
            u32 random_choice;
            if (1) { //(door_up || door_down) {
                random_choice = random_number_table[random_number_index++] % 2;
            } else {
                random_choice = random_number_table[random_number_index++] % 3;
            }
            b32 created_z_door = false;
            if (random_choice == 2) {
                created_z_door = true;
                if (abs_tile_z == screen_base_z) {
                    door_up = true;
                } else {
                    door_down = true;
                }
            } else if (random_choice == 1) {
                door_right = true;
            } else {
                door_top = true;
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
                door_up = false;
                door_down = false;
            }
            
            door_right = false;
            door_top = false;

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
        
        memory->initialized = true;
    }

    world_t *world = state->world;
    f32 meters_to_pixels = state->meters_to_pixels;

    /////////////////////////////////
    // NOTE(xkazu0x): gamepad control
    for (u32 gamepad_index = 0; gamepad_index < ARRAY_COUNT(input->gamepads); gamepad_index++) {
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
            
            controlled_player->dd_pos = make_vec2(gamepad->left_stick.x, gamepad->left_stick.y);

            // NOTE(xkazu0x): combine keyboard and gamepad inputs
            // only for the first player
            if (gamepad_index == 1) {
                if (input->keyboard[KEY_W].down) controlled_player->dd_pos.y = 1.0f;
                if (input->keyboard[KEY_S].down) controlled_player->dd_pos.y = -1.0f;
                if (input->keyboard[KEY_A].down) controlled_player->dd_pos.x = -1.0f;
                if (input->keyboard[KEY_D].down) controlled_player->dd_pos.x = 1.0f;
                
                if (input->keyboard[KEY_UP].pressed) controlled_player->d_sword = make_vec2(0.0f, 1.0f);
                if (input->keyboard[KEY_DOWN].pressed) controlled_player->d_sword = make_vec2(0.0f, -1.0f);
                if (input->keyboard[KEY_LEFT].pressed) controlled_player->d_sword = make_vec2(-1.0f, 0.0f);
                if (input->keyboard[KEY_RIGHT].pressed) controlled_player->d_sword = make_vec2(1.0f, 0.0f);

                if (input->keyboard[KEY_SPACE].pressed) controlled_player->d_z = 3.0f;
            }
        }
    }

    //////////////////////////////////
    // NOTE(xkazu0x): simulation begin
    u32 tile_span_x = 17*3;
    u32 tile_span_y = 9*3;
    rect2 camera_bounds = rect2_center_dim(make_vec2(0.0f), world->tile_side_in_meters*make_vec2((f32)tile_span_x, (f32)tile_span_y));
    
    memory_arena_t sim_arena = memory_arena_create(memory->transient_storage_size, memory->transient_storage);
    sim_region_t *sim_region = begin_sim(&sim_arena, state, world, state->camera_pos, camera_bounds);

    ///////////////////////////////////
    // NOTE(xkazu0x): update and render
    for (u32 y = 0; y < 12; y++) {
        for (u32 x = 0; x < 20; x++) {
            vec3 color = make_vec3(57.0f/255.0f, 44.0f/255.0f, 49.0f/255.0f);
            
            if (((y % 2 == 0) && (x % 2 == 0)) ||
                ((y % 2 == 1) && (x % 2 == 1))) {
                color = make_vec3(74.0f/255.0f, 60.0f/255.0f, 74.0f/255.0f);
            }
            
            vec2 min = make_vec2(x*tile_size_in_pixels, y*tile_size_in_pixels);
            vec2 max = make_vec2(min.x + tile_size_in_pixels, min.y + tile_size_in_pixels);
            
            draw_rect(framebuffer, min, max, color);
        }
    }
    
    f32 screen_center_x = 0.5f*((f32)framebuffer->width);
    f32 screen_center_y = 0.5f*((f32)framebuffer->height);

    // TODO(xkazu0x): move this out into excalibur_entity.cpp
    entity_visible_piece_group_t piece_group;
    piece_group.game_state = state;

    sim_entity_t *entity = sim_region->entities;
    for (u32 entity_index = 0;
         entity_index < sim_region->entity_count;
         ++entity_index, ++entity)
    {
        if (entity->updatable) {
            piece_group.piece_count = 0;
            f32 delta = clock->delta_seconds;

            f32 shadow_alpha = 1.0f - 0.5f*entity->z;
            if (shadow_alpha < 0.0f) shadow_alpha = 0.0f;

            move_spec_t move_spec = default_move_spec();
            vec2 dd_pos = make_vec2(0.0f);
            
            switch (entity->type) {
                case ENTITY_TYPE_PLAYER: {
                    for (u32 controlled_index = 0;
                         controlled_index < ARRAY_COUNT(state->controlled_players);
                         ++controlled_index) {
                        controlled_player_t *controlled_player = state->controlled_players + controlled_index;
                        if (entity->storage_index == controlled_player->entity_index) {
                            if (controlled_player->d_z != 0.0f) {
                                entity->d_z = controlled_player->d_z;
                            }

                            move_spec.unit_max_accel_vector = true;
                            move_spec.speed = 50.0f;
                            move_spec.drag = 5.0f;
                            dd_pos = controlled_player->dd_pos;
                            
                            if ((controlled_player->d_sword.x != 0.0f) || (controlled_player->d_sword.y != 0.0f)) {
                                sim_entity_t *sword = entity->sword.ptr;
                                if (sword && is_entity_flag_set(sword, ENTITY_FLAG_NON_SPATIAL)) {
                                    sword->distance_limit = 3.0f;
                                    make_entity_spatial(sword, entity->pos, 10.0f*controlled_player->d_sword);
                                    add_collision_rule(state, sword->storage_index, entity->storage_index, false);
                                }
                            }
                        }
                    }

                    bitmap_t *sprite = &state->player_sprites[entity->direction];
                    push_bitmap(&piece_group, &state->shadow_sprite, make_vec3(0.0f), shadow_alpha, 0.0f);
                    push_bitmap(&piece_group, sprite, make_vec3(0.0f, 0.0f, -0.4f));
                    draw_hit_points(&piece_group, entity);
                } break;
                case ENTITY_TYPE_WALL: {
                    push_bitmap(&piece_group, &state->wall_sprite, make_vec3(0.0f));
                } break;
                case ENTITY_TYPE_SWORD: {
                    move_spec.unit_max_accel_vector = false;
                    move_spec.speed = 0.0f;
                    move_spec.drag = 0.0f;

                    // TODO(xkazu0x): IMPORTANT(xkazu0x): add the ability in the collision
                    // routines to  understand a movement limit for an entity, and
                    // then update this routine to use that to know when to kill the
                    // sword.
                    // TODO(xkazu0x): need to handle the fact that distance_traveled
                    // might not have enough distance for the total entity move
                    // for the frame
                    if (entity->distance_limit == 0.0f) {
                        clear_collision_rules_for(state, entity->storage_index);
                        make_entity_non_spatial(entity);
                    }

                    push_bitmap(&piece_group, &state->shadow_sprite, make_vec3(0.0f));
                } break;
                case ENTITY_TYPE_FAMILIAR: {
                    sim_entity_t *closest_player = 0;
                    f32 closest_player_distance_sqr = square(10.0f); // NOTE(xkazu0x): ten meter maximun search!

                    // TODO(xkazu0x): make spatial queries easy for things!
                    sim_entity_t *test_entity = sim_region->entities;
                    for (u32 test_entity_index = 0;
                         test_entity_index < sim_region->entity_count;
                         ++test_entity_index, ++test_entity) {
                        if (test_entity->type == ENTITY_TYPE_PLAYER) {
                            f32 test_distance_sqr = vec_length_square(test_entity->pos - entity->pos);
                            if (closest_player_distance_sqr > test_distance_sqr) {
                                closest_player = test_entity;
                                closest_player_distance_sqr = test_distance_sqr;
                            }
                        }
                    }

                    if ((closest_player) && (closest_player_distance_sqr > square(2.5f))) {
                        f32 acceleration = 0.5f;
                        f32 lenght_normalized = acceleration / square_root(closest_player_distance_sqr);
                        dd_pos = lenght_normalized*(closest_player->pos - entity->pos);
                    }

                    move_spec.unit_max_accel_vector = true;
                    move_spec.speed = 50.0f;
                    move_spec.drag = 5.0f;
    
                    if (dd_pos.x == 0.0f && dd_pos.y == 0.0f) {
                        entity->direction = 2;
                    }
                
                    entity->t_bob += delta;
                    if (entity->t_bob > (2.0f*pi32)) {
                        entity->t_bob -= (2.0f*pi32);
                    }

                    f32 bob_sin = sin_f32(5.0f*entity->t_bob);
                
                    push_bitmap(&piece_group, &state->shadow_sprite, make_vec3(0.0f), (0.5f*shadow_alpha) + 0.2f*bob_sin, 0.0f);
                    push_bitmap(&piece_group, &state->bat_sprite, make_vec3(0.0f, 0.0f, 0.2f*bob_sin - 0.6f));
                } break;
                case ENTITY_TYPE_MONSTER: {
                    push_bitmap(&piece_group, &state->shadow_sprite, make_vec3(0.0f), shadow_alpha, 0.0f);
                    push_bitmap(&piece_group, &state->player_sprites[2], make_vec3(0.0f, 0.0f, -0.4f));
                    draw_hit_points(&piece_group, entity);
                } break;
                default: {
                    INVALID_CODE_PATH();
                }
            }

            if (!is_entity_flag_set(entity, ENTITY_FLAG_NON_SPATIAL)) {
                move_entity(state, sim_region, entity, clock->delta_seconds, &move_spec, dd_pos);
            }
            
            f32 entity_ground_point_x = screen_center_x + meters_to_pixels*entity->pos.x;
            f32 entity_ground_point_y = screen_center_y - meters_to_pixels*entity->pos.y;
            f32 entity_z = -meters_to_pixels*entity->z;
        
#if 0
            vec2 entity_left_top = make_vec2(entity_ground_point_x - 0.5f*meters_to_pixels*low_entity->sim.width,
                                           entity_ground_point_y - 0.5f*meters_to_pixels*low_entity->sim.height);
            vec2 entity_width_height = make_vec2(low_entity->sim.width, low_entity->sim.height);
            draw_rectangle(framebuffer,
                           entity_left_top,
                           entity_left_top + meters_to_pixels*entity_width_height,
                           make_vec3(0.0f, 0.0f, 1.0f));
#endif

            for (u32 piece_index = 0; piece_index < piece_group.piece_count; piece_index++) {
                entity_visible_piece_t *piece = piece_group.pieces + piece_index;
                vec2 center = make_vec2(entity_ground_point_x + piece->offset.x,
                                      entity_ground_point_y + piece->offset.y + piece->offset.z + piece->entity_zc*entity_z);
                if (piece->bitmap) {
                    draw_bitmap(framebuffer, piece->bitmap, center.x, center.y, piece->color.a);
                } else {
                    vec2 half_size = 0.5f*meters_to_pixels*piece->size;
                    vec3 color = make_vec3(piece->color.r, piece->color.g, piece->color.b);
                    draw_rect(framebuffer, center - half_size, center + half_size, color);
                }
            }
        }
    }
    
    ////////////////////////////////
    // NOTE(xkazu0x): simulation end
    world_position_t world_origin = {};
    vec3 delta_origin = subtract(sim_region->world, &world_origin, &sim_region->origin);
    draw_rect(framebuffer, make_vec2(delta_origin.x, delta_origin.y), make_vec2(tile_size_in_pixels), make_vec3(1.0f));
    
    end_sim(sim_region, state);
}
