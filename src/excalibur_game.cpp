#include "base/excalibur_base.h"
#include "os/excalibur_os.h"
#include "excalibur_random.h"
#include "excalibur_world.h"
#include "excalibur_simulation.h"
#include "excalibur_entity.h"

struct Arena {
    memi reserve_size;
    memi used;
    u8 *base_memory;

    s32 temp_count;
};

internal Arena
make_arena(memi reserve_size, void *base_memory) {
    Arena result;
    result.reserve_size = reserve_size;
    result.used = 0;
    result.base_memory = (u8 *)base_memory;
    result.temp_count = 0;
    return(result);
}

internal void *
arena_push_(Arena *arena, memi size) {
    assert((arena->used + size) <= arena->reserve_size);
    void *result = arena->base_memory + arena->used;
    arena->used += size;
    return(result);
}

#define push_size(arena, size) arena_push_(arena, size)
#define push_struct(arena, type) (type *)arena_push_(arena, sizeof(type))
#define push_array(arena, type, count) (type *)arena_push_(arena, sizeof(type)*(count))

struct Temp_Memory {
    Arena *arena;
    memi used;
};

internal Temp_Memory
begin_temp_memory(Arena *arena) {
    Temp_Memory result;
    result.arena = arena;
    result.used = arena->used;
    ++arena->temp_count;
    return(result);
}

internal void
end_temp_memory(Temp_Memory *temp) {
    Arena *arena = temp->arena;
    
    assert(arena->used >= temp->used);
    arena->used = temp->used;

    assert(arena->temp_count > 0);
    --arena->temp_count;
}

internal void
check_arena(Arena *arena) {
    assert(arena->temp_count == 0);
}

internal inline void
zero_size(memi size, void *ptr) {
    u8 *byte = (u8 *)ptr;
    while (size--) {
        *byte++ = 0;
    }
}

#define zero_struct(instance) zero_size(sizeof(instance), &(instance))

#include "excalibur_render.h"
#include "excalibur_game.h"

#include "base/excalibur_base.cpp"
#include "excalibur_world.cpp"
#include "excalibur_simulation.cpp"
#include "excalibur_entity.cpp"
#include "excalibur_render.cpp"

internal Gamepad *
get_gamepad(OS_Input *input, u32 index) {
    assert(index < Gamepad_Count);
    Gamepad *result = 0;
    if (index < Gamepad_Count) {
        result = &input->gamepads[index];
    }
    return(result);
}

#pragma pack(push, 1)
struct Bitmap_Header {
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

internal Vec2
top_down_align(Bitmap *bitmap, Vec2 align) {
    //align.y = (f32)(bitmap->height - 1) - align.y;
    align.y = (f32)bitmap->height - align.y;
    
    align.x = safe_ratio0(align.x, (f32)bitmap->width);
    align.y = safe_ratio0(align.y, (f32)bitmap->height);
    
    return(align);
}

internal void
set_bitmap_align(Bitmap *bitmap, Vec2 align) {
    align = top_down_align(bitmap, align);
    bitmap->align_percentage = align;
}

internal Bitmap
debug_load_bitmap(Debug_OS_Read_File *debug_os_read_file, OS_Thread *thread, char *filename, s32 align_x, s32 align_y) {
    Bitmap result = {};
    
    Debug_OS_File file = debug_os_read_file(thread, filename);
    if (file.size != 0) {
        Bitmap_Header *header = (Bitmap_Header *)file.data;
        u32 *pixels = (u32 *)((u8 *)file.data + header->bitmap_offset);
        
        result.memory = pixels;
        result.width = header->width;
        result.height = header->height;
        result.align_percentage = top_down_align(&result, make_vec2((f32)align_x, (f32)align_y));
        result.width_over_height = safe_ratio0((f32)result.width, (f32)result.height);
        
        assert(result.height >= 0);
        assert(header->compression == 3);
        
        // NOTE(xkazu0x): byte order in memory is determined bu the header itself,
        // so we have to read out the masks and convert the pixels ourselves.
        u32 red_mask = header->red_mask;
        u32 green_mask = header->green_mask;
        u32 blue_mask = header->blue_mask;
        u32 alpha_mask = ~(red_mask | green_mask | blue_mask);
        
        Bit_Scan red_scan = find_least_significant_set_bit(red_mask);
        Bit_Scan green_scan = find_least_significant_set_bit(green_mask);
        Bit_Scan blue_scan = find_least_significant_set_bit(blue_mask);
        Bit_Scan alpha_scan = find_least_significant_set_bit(alpha_mask);

        assert(red_scan.found);
        assert(green_scan.found);
        assert(blue_scan.found);
        assert(alpha_scan.found);

        s32 red_shift_down   = (s32)red_scan.index;
        s32 green_shift_down = (s32)green_scan.index;
        s32 blue_shift_down  = (s32)blue_scan.index;
        s32 alpha_shift_down = (s32)alpha_scan.index;

        u32 *source_dest = pixels;
        for (s32 y = 0;
             y < header->height;
             ++y) {
            for (s32 x = 0;
                 x < header->width;
                 ++x) {
                u32 color = *source_dest;

                Vec4 texel = make_vec4((f32)((color & red_mask)   >> red_shift_down),
                                       (f32)((color & green_mask) >> green_shift_down),
                                       (f32)((color & blue_mask)  >> blue_shift_down),
                                       (f32)((color & alpha_mask) >> alpha_shift_down));
                
                texel = srgb255_to_linear1(texel);
                texel.rgb *= texel.a;
                texel = linear1_to_srgb255(texel);
                
                *source_dest++ = (((u32)(texel.a + 0.5f) << 24) |
                                  ((u32)(texel.r + 0.5f) << 16) |
                                  ((u32)(texel.g + 0.5f) << 8) |
                                  ((u32)(texel.b + 0.5f) << 0));
            }
        }
    }

    result.pitch = result.width*BYTES_PER_PIXEL;

#if 0
    result.memory = (u8 *)result.memory + result.pitch*(result.height - 1);
    result.pitch = -result.pitch;
#endif
    
    return(result);
}

internal Bitmap
debug_load_bitmap(Debug_OS_Read_File *debug_os_read_file, OS_Thread *thread, char *filename) {
    Bitmap result = debug_load_bitmap(debug_os_read_file, thread, filename, 0, 0);
    result.align_percentage = make_vec2(0.5f);
    return(result);
}

struct Add_Low_Entity_Result {
    u32 low_index;
    Low_Entity *low;
};

internal Add_Low_Entity_Result
add_low_entity(Game_State *game_state, Entity_Type type, World_Position pos) {
    assert(game_state->low_entity_count < array_count(game_state->low_entities));
    u32 entity_index = game_state->low_entity_count++;
    
    Low_Entity *low_entity = game_state->low_entities + entity_index;
    *low_entity = {};
    low_entity->sim.type = type;
    low_entity->sim.collision = game_state->null_collision;
    low_entity->pos = null_position();

    change_entity_location(&game_state->world_arena, game_state->world, entity_index, low_entity, pos);
    
    Add_Low_Entity_Result result = {};
    result.low_index = entity_index;
    result.low = low_entity;
    
    return(result);
}

internal Add_Low_Entity_Result
add_grounded_entity(Game_State *game_state, Entity_Type type, World_Position pos,
                    Sim_Entity_Collision_Volume_Group *collision) {
    Add_Low_Entity_Result entity = add_low_entity(game_state, type, pos);
    entity.low->sim.collision = collision;
    return(entity);
}

inline World_Position
chunk_position_from_tile_position(World *world, s32 tile_x, s32 tile_y, s32 tile_z,
                                  Vec3 additional_offset = make_vec3(0.0f)) {
    World_Position base_pos = {};

    f32 tile_side_in_meters = 1.4f;
    f32 tile_depth_in_meters = 3.0f;
    
    Vec3 tile_dim = make_vec3(tile_side_in_meters, tile_side_in_meters, tile_depth_in_meters);
    Vec3 offset = hadamard_product(tile_dim, make_vec3((f32)tile_x, (f32)tile_y, (f32)tile_z));
    World_Position result = map_into_chunk_space(world, base_pos, additional_offset + offset);
    
    assert(is_canonical(world, result.offset_));
    
    return(result);
}

internal Add_Low_Entity_Result
add_standard_room(Game_State *game_state, u32 tile_x, u32 tile_y, u32 tile_z) {
    World_Position pos = chunk_position_from_tile_position(game_state->world, tile_x, tile_y, tile_z);
    
    Add_Low_Entity_Result entity = add_grounded_entity(game_state, EntityType_Space, pos,
                                                       game_state->standard_room_collision);
    add_entity_flags(&entity.low->sim, EntityFlag_Traversable);
    
    return(entity);
}

internal Add_Low_Entity_Result
add_wall(Game_State *game_state, u32 tile_x, u32 tile_y, u32 tile_z) {
    World_Position pos = chunk_position_from_tile_position(game_state->world, tile_x, tile_y, tile_z);
    
    Add_Low_Entity_Result entity = add_grounded_entity(game_state, EntityType_Wall, pos,
                                                       game_state->wall_collision);
    add_entity_flags(&entity.low->sim, EntityFlag_Collides);
    
    return(entity);
}

internal Add_Low_Entity_Result
add_stair(Game_State *game_state, u32 tile_x, u32 tile_y, u32 tile_z) {
    World_Position pos = chunk_position_from_tile_position(game_state->world, tile_x, tile_y, tile_z);
    
    Add_Low_Entity_Result entity = add_grounded_entity(game_state, EntityType_Stairwell, pos,
                                                       game_state->stair_collision);
    entity.low->sim.walkable_dim = entity.low->sim.collision->total_volume.dim.xy;
    entity.low->sim.walkable_height = game_state->typical_floor_height;
    
    add_entity_flags(&entity.low->sim, EntityFlag_Collides);
    
    return(entity);
}

internal void
init_hit_points(Low_Entity *low_entity, u32 hit_point_count) {
    assert(hit_point_count < array_count(low_entity->sim.hit_points));
    low_entity->sim.hit_point_max = hit_point_count;
    for (u32 hit_point_index = 0;
         hit_point_index < hit_point_count;
         ++hit_point_index) {
        Hit_Point *hit_point = low_entity->sim.hit_points + hit_point_index;
        hit_point->flags = 0;
        hit_point->filled_amount = HIT_POINT_FILLED_MAX;
    }
}

internal Add_Low_Entity_Result
add_sword(Game_State *game_state) {
    Add_Low_Entity_Result entity = add_low_entity(game_state, EntityType_Sword, null_position());
    entity.low->sim.collision = game_state->sword_collision;
    
    add_entity_flags(&entity.low->sim, EntityFlag_Moveable);
    
    return(entity);
}

internal Add_Low_Entity_Result
add_player(Game_State *game_state) {
    World_Position pos = game_state->camera_pos;
    
    Add_Low_Entity_Result entity = add_grounded_entity(game_state, EntityType_Player, pos,
                                                       game_state->player_collision);
    add_entity_flags(&entity.low->sim, EntityFlag_Collides | EntityFlag_Moveable);
    init_hit_points(entity.low, 3);

    Add_Low_Entity_Result sword = add_sword(game_state);
    entity.low->sim.sword.index = sword.low_index;
    
    if (game_state->camera_following_entity_index == 0) {
        game_state->camera_following_entity_index = entity.low_index;
    }
    
    return(entity);
}

internal Add_Low_Entity_Result
add_monster(Game_State *game_state, u32 tile_x, u32 tile_y, u32 tile_z) {
    World_Position pos = chunk_position_from_tile_position(game_state->world, tile_x, tile_y, tile_z);
    
    Add_Low_Entity_Result entity = add_grounded_entity(game_state, EntityType_Monster, pos,
                                                       game_state->monster_collision);
    add_entity_flags(&entity.low->sim, EntityFlag_Collides | EntityFlag_Moveable);
    init_hit_points(entity.low, 5);
    
    return(entity);
}

internal Add_Low_Entity_Result
add_familiar(Game_State *game_state, u32 tile_x, u32 tile_y, u32 tile_z) {
    World_Position pos = chunk_position_from_tile_position(game_state->world, tile_x, tile_y, tile_z);
    
    Add_Low_Entity_Result entity = add_grounded_entity(game_state, EntityType_Familiar, pos,
                                                       game_state->familiar_collision);
    add_entity_flags(&entity.low->sim, EntityFlag_Collides | EntityFlag_Moveable);

    return(entity);
}

internal void
draw_hit_points(Render_Group *piece_group, Sim_Entity *entity) {
    if (entity->hit_point_max >= 1) {
        Vec2 health_dim = make_vec2(0.125f*1.4f);
        f32 spacing_x = 1.5f*health_dim.x;
        
        Vec2 hit_pos = make_vec2((-0.5f*(entity->hit_point_max - 1)*spacing_x), 1.0f);
        Vec2 hit_dpos = make_vec2(spacing_x, 0.0f);
                        
        for (u32 health_index = 0;
             health_index < entity->hit_point_max;
             ++health_index) {
            Hit_Point *hit_point = entity->hit_points + health_index;
            Vec4 color = make_rgba(222.0f, 214.0f, 222.0f, 255.0f);
            
            if (hit_point->filled_amount == 0) {
                color = make_rgba(164.0f, 157.0f, 164.0f, 255.0f);
            }
            
            render_rect(piece_group, make_vec3(hit_pos, 0.0f), health_dim, color);
            hit_pos += hit_dpos;
        }
    }
}

internal void
clear_collision_rules_for(Game_State *game_state, u32 storage_index) {
    // TODO(xkazu0x): need to make a better data structure that allows
    // removal of collision rules without searching the entire table
    // NOTE(xkazu0x): one way to make removal easy would be to always
    // add _both_ orders of the pairs of storage indices to the
    // hash table, so no matter which position the entity is in,
    // you can always find it. then, when you do your first pass
    // through for removal, you just remember the original top
    // of the free list, and when you're done, do a pass through all
    // the new things on the free list, and remove the reverse of
    // those pairs.
    for (u32 hash_bucket = 0;
         hash_bucket < array_count(game_state->collision_rule_hash);
         ++hash_bucket) {
        for (Pairwise_Collision_Rule **rule = &game_state->collision_rule_hash[hash_bucket];
             *rule;
             ) {
            if (((*rule)->storage_index_a == storage_index) ||
                ((*rule)->storage_index_b == storage_index)) {
                Pairwise_Collision_Rule *removed_rule = *rule;
                *rule = (*rule)->next_in_hash;
                
                removed_rule->next_in_hash = game_state->first_free_collision_rule;
                game_state->first_free_collision_rule = removed_rule;
            } else {
                rule = &(*rule)->next_in_hash;
            }
        }
    }
}

internal void
add_collision_rule(Game_State *game_state, u32 storage_index_a, u32 storage_index_b, b32 can_collide) {
    // TODO(xkazu0x): collapse this with should_collide
    if (storage_index_a > storage_index_b) {
        u32 temp = storage_index_a;
        storage_index_a = storage_index_b;
        storage_index_b = temp;
    }

    // TODO(xkazu0x): BETTER HASH FUCTION
    Pairwise_Collision_Rule *found = 0;
    u32 hash_bucket = storage_index_a & (array_count(game_state->collision_rule_hash) - 1);
    for (Pairwise_Collision_Rule *rule = game_state->collision_rule_hash[hash_bucket];
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
        found = game_state->first_free_collision_rule;
        if (found) {
            game_state->first_free_collision_rule = found->next_in_hash;
        } else {
            found = push_struct(&game_state->world_arena, Pairwise_Collision_Rule);
        }
        
        found->next_in_hash = game_state->collision_rule_hash[hash_bucket];
        game_state->collision_rule_hash[hash_bucket] = found;
    }

    if (found) {
        found->storage_index_a = storage_index_a;
        found->storage_index_b = storage_index_b;
        found->can_collide = can_collide;
    }
}

internal Sim_Entity_Collision_Volume_Group *
make_simple_grounded_collision(Game_State *game_state, f32 dim_x, f32 dim_y, f32 dim_z) {
    // TODO(xkazu0x): NOT WORLD ARENA! change to using the fundamental types arena.
    Sim_Entity_Collision_Volume_Group *collision = push_struct(&game_state->world_arena, Sim_Entity_Collision_Volume_Group);
    collision->volume_count = 1;
    collision->volumes = push_array(&game_state->world_arena, Sim_Entity_Collision_Volume, collision->volume_count);
    collision->total_volume.offset = make_vec3(0.0f, 0.0f, 0.5f*dim_z);
    collision->total_volume.dim = make_vec3(dim_x, dim_y, dim_z);
    collision->volumes[0] = collision->total_volume;
    return(collision);
}

internal Sim_Entity_Collision_Volume_Group *
make_null_collision(Game_State *game_state) {
    // TODO(xkazu0x): NOT WORLD ARENA! change to using the fundamental types arena.
    Sim_Entity_Collision_Volume_Group *collision = push_struct(&game_state->world_arena, Sim_Entity_Collision_Volume_Group);
    collision->volume_count = 0;
    collision->volumes = 0;
    collision->total_volume.offset = make_vec3(0.0f);
    // TODO(xkazu0x): should this be negative?
    collision->total_volume.dim = make_vec3(0.0f);
    return(collision);
}

internal void
fill_ground_chunk(Transient_State *tran_state, Game_State *game_state, Ground_Buffer *ground_buffer, World_Position *position) {
    Temp_Memory render_memory = begin_temp_memory(&tran_state->arena);

    // TODO(xkazu0x): Need to be able to set a orthographic display mode!!!
    Bitmap *output_target = &ground_buffer->bitmap;
    output_target->align_percentage = make_vec2(0.5f);
    output_target->width_over_height = 1.0f;
    
    Render_Group *render_group = render_group_alloc(&tran_state->arena, MB(4), output_target->width, output_target->height);

    render_clear(render_group, make_vec4(1.0f, 0.0f, 1.0f, 1.0f));
    
    ground_buffer->position = *position;
    
    f32 width = game_state->world->chunk_dim_in_meters.x;
    f32 height = game_state->world->chunk_dim_in_meters.y;
    Vec2 half_dim = 0.5f*make_vec2(width, height);

    half_dim = 1.5f*half_dim;
    
    for (s32 chunk_offset_y = -1;
         chunk_offset_y <= 1;
         ++chunk_offset_y) {
        for (s32 chunk_offset_x = -1;
             chunk_offset_x <= 1;
             ++chunk_offset_x) {
            s32 chunk_x = position->chunk_x + chunk_offset_x;
            s32 chunk_y = position->chunk_y + chunk_offset_y;
            u32 chunk_z = position->chunk_z;
            
            // TODO(xkazu0x): look into wang hashing here or some other spatial seed generation "thing"!
            random_series_t series = random_seed(464*chunk_x + 132*chunk_y + 235*chunk_z);
            Vec2 chunk_center = make_vec2(chunk_offset_x*width, chunk_offset_y*height);

            // TODO(xkazu0x): optimize to draw ground index
            for (u32 ground_index = 0;
                 ground_index < 32;
                 ++ground_index) {
                Bitmap *sprite;
                if (random_choice(&series, 2)) {
                    sprite = game_state->grass_sprites + random_choice(&series, array_count(game_state->grass_sprites));
                } else {
                    sprite = game_state->stone_sprites + random_choice(&series, array_count(game_state->stone_sprites));
                }

                Vec2 output_offset = hadamard_product(half_dim, make_vec2(random_bilateral(&series), random_bilateral(&series)));
                Vec2 sprite_offset = chunk_center + output_offset;
        
                render_bitmap(render_group, sprite, make_vec3(sprite_offset, 0.0f), 5.0f);
            }
        }
    }
    
    for (s32 chunk_offset_y = -1;
         chunk_offset_y <= 1;
         ++chunk_offset_y) {
        for (s32 chunk_offset_x = -1;
             chunk_offset_x <= 1;
             ++chunk_offset_x) {
            s32 chunk_x = position->chunk_x + chunk_offset_x;
            s32 chunk_y = position->chunk_y + chunk_offset_y;
            u32 chunk_z = position->chunk_z;
            
            // TODO(xkazu0x): look into wang hashing here or some other spatial seed generation "thing"!
            random_series_t series = random_seed(464*chunk_x + 132*chunk_y + 235*chunk_z);
            Vec2 chunk_center = make_vec2(chunk_offset_x*width, chunk_offset_y*height);
                        
            for (u32 ground_index = 0;
                 ground_index < 8;
                 ++ground_index) {
                Bitmap *sprite;
                if (random_choice(&series, 2)) {
                    sprite = game_state->tuft_sprites + random_choice(&series, array_count(game_state->tuft_sprites));
                } else {
                    sprite = game_state->tuft_sprites + random_choice(&series, array_count(game_state->tuft_sprites));
                }
        
                Vec2 output_offset = hadamard_product(half_dim, make_vec2(random_bilateral(&series), random_bilateral(&series)));
                Vec2 sprite_offset = chunk_center + output_offset;
        
                render_bitmap(render_group, sprite, make_vec3(sprite_offset, 0.0f), 5.0f);
            }
        }
    }
    
    render_group_draw(render_group, output_target);
    end_temp_memory(&render_memory);
}

internal void
clear_bitmap(Bitmap *bitmap) {
    if (bitmap->memory) {
        s32 bitmap_size = bitmap->width*bitmap->height*BYTES_PER_PIXEL;
        zero_size(bitmap_size, bitmap->memory);
    }
}

internal Bitmap
make_empty_bitmap(Arena *arena, u32 width, u32 height, b32 clear = true) {
    Bitmap result = {};
    result.width = width;
    result.height = height;
    result.pitch = result.width*BITMAP_BYTES_PER_PIXEL;
    s32 bitmap_size = width*height*BITMAP_BYTES_PER_PIXEL;
    result.memory = push_size(arena, bitmap_size);
    if (clear) {
        clear_bitmap(&result);
    }
    
    return(result);
}

internal void
make_sphere_normal_map(Bitmap *bitmap, f32 roughness, f32 cx = 1.0f, f32 cy = 1.0f) {
    f32 inv_width = 1.0f/(f32)(bitmap->width - 1);
    f32 inv_height = 1.0f/(f32)(bitmap->height - 1);

    u8 *row = (u8 *)bitmap->memory;
    for (s32 y = 0;
         y < bitmap->height;
         ++y) {
        u32 *pixel = (u32 *)row;
        for (s32 x = 0;
             x < bitmap->width;
             ++x) {
            Vec2 bitmap_coord = make_vec2(inv_width*(f32)x, inv_height*(f32)y);

            f32 normal_x = cx*(2.0f*bitmap_coord.u - 1.0f);
            f32 normal_y = cy*(2.0f*bitmap_coord.v - 1.0f);
            f32 normal_z = 0.0f;
            
            f32 root_term = 1.0f - square(normal_x) - square(normal_y);
            Vec3 normal = make_vec3(0.0f, 0.707106781188f, 0.707106781188f);

            if (root_term >= 0.0f) {
                normal_z = square_root(root_term);
                normal = make_vec3(normal_x, normal_y, normal_z);
            }

            Vec4 color = make_vec4(255.0f*(0.5f*(normal.x + 1.0f)),
                                   255.0f*(0.5f*(normal.y + 1.0f)),
                                   255.0f*(0.5f*(normal.z + 1.0f)),
                                   255.0f*roughness);
            
            *pixel++ = (((u32)(color.a + 0.5f) << 24) |
                        ((u32)(color.r + 0.5f) << 16) |
                        ((u32)(color.g + 0.5f) << 8) |
                        ((u32)(color.b + 0.5f) << 0));
        }
        row += bitmap->pitch;
    }
}

internal void
make_sphere_diffuse_map(Bitmap *bitmap, f32 cx = 1.0f, f32 cy = 1.0f) {
    f32 inv_width = 1.0f/(f32)(bitmap->width - 1);
    f32 inv_height = 1.0f/(f32)(bitmap->height - 1);

    u8 *row = (u8 *)bitmap->memory;
    for (s32 y = 0;
         y < bitmap->height;
         ++y) {
        u32 *pixel = (u32 *)row;
        for (s32 x = 0;
             x < bitmap->width;
             ++x) {
            Vec2 bitmap_coord = make_vec2(inv_width*(f32)x, inv_height*(f32)y);

            f32 normal_x = cx*(2.0f*bitmap_coord.u - 1.0f);
            f32 normal_y = cy*(2.0f*bitmap_coord.v - 1.0f);

            f32 root_term = 1.0f - square(normal_x) - square(normal_y);
            f32 alpha = 0.0f;
            
            if (root_term >= 0.0f) {
                alpha = 1.0f;
            }

            Vec3 base_color = make_vec3(0.0f);
            alpha *= 255.0f;
            
            Vec4 color = make_vec4(alpha*base_color.x,
                                   alpha*base_color.y,
                                   alpha*base_color.z,
                                   alpha);
            
            *pixel++ = (((u32)(color.a + 0.5f) << 24) |
                        ((u32)(color.r + 0.5f) << 16) |
                        ((u32)(color.g + 0.5f) << 8) |
                        ((u32)(color.b + 0.5f) << 0));
        }
        row += bitmap->pitch;
    }
}

internal void
make_pyramid_normal_map(Bitmap *bitmap, f32 roughness) {
    f32 inv_width = 1.0f/(f32)(bitmap->width - 1);
    f32 inv_height = 1.0f/(f32)(bitmap->height - 1);

    u8 *row = (u8 *)bitmap->memory;
    for (s32 y = 0;
         y < bitmap->height;
         ++y) {
        u32 *pixel = (u32 *)row;
        for (s32 x = 0;
             x < bitmap->width;
             ++x) {
            Vec2 bitmap_coord = make_vec2(inv_width*(f32)x, inv_height*(f32)y);

            s32 inv_x = (bitmap->width - 1) - x;
            f32 seven = 0.707106781188f;
            Vec3 normal = make_vec3(0.0f, 0.0f, seven);
            if (x < y) {
                if (inv_x < y) {
                    normal.x = -seven;
                } else {
                    normal.y = seven;
                }
            } else {
                if (inv_x < y) {
                    normal.y = -seven;
                } else {
                    normal.x = seven;
                }
            }
            
            Vec4 color = make_vec4(255.0f*(0.5f*(normal.x + 1.0f)),
                                   255.0f*(0.5f*(normal.y + 1.0f)),
                                   255.0f*(0.5f*(normal.z + 1.0f)),
                                   255.0f*roughness);
            
            *pixel++ = (((u32)(color.a + 0.5f) << 24) |
                        ((u32)(color.r + 0.5f) << 16) |
                        ((u32)(color.g + 0.5f) << 8) |
                        ((u32)(color.b + 0.5f) << 0));
        }
        row += bitmap->pitch;
    }
}

shared_function
GAME_UPDATE_AND_RENDER(game_update_and_render) {
    assert(sizeof(Game_State) <= memory->permanent_storage_size);
    Game_State *game_state = (Game_State *)memory->permanent_storage;
    
    u32 tile_count_x = 17;
    u32 tile_count_y = 9;

    f32 tile_side_in_meters = 1.4f;
    f32 tile_depth_in_meters = 3.0f;
    
    u32 ground_buffer_width = 256;
    u32 ground_buffer_height = 256;

    //
    // NOTE(xkazu0x): initialize game state
    //
    if (!memory->initialized) {
        game_state->typical_floor_height = 3.0f;

        // TODO(xkazu0x): Remove this!
        //f32 pixels_to_meters = 1.0f / 11.4285714286f; // tile_size_in_pixels/1.4f (tile_size_in_meters);
        f32 pixels_to_meters = 1.0f / 42.0f;
        Vec3 world_chunk_dim_in_meters = make_vec3(pixels_to_meters*(f32)ground_buffer_width,
                                                   pixels_to_meters*(f32)ground_buffer_height,
                                                   game_state->typical_floor_height);
            
        // TODO(xkazu0x): start partioning memory space
        game_state->world_arena = make_arena(memory->permanent_storage_size - sizeof(Game_State),
                                        (u8 *)memory->permanent_storage + sizeof(Game_State));
        game_state->world = push_struct(&game_state->world_arena, World);
        
        World *world = game_state->world;
        initialize_world(world, world_chunk_dim_in_meters);
        
        game_state->null_collision =
            make_null_collision(game_state);
        game_state->wall_collision =
            make_simple_grounded_collision(game_state,
                                           tile_side_in_meters,
                                           tile_side_in_meters,
                                           tile_depth_in_meters);
        game_state->standard_room_collision =
            make_simple_grounded_collision(game_state,
                                           tile_count_x*tile_side_in_meters,
                                           tile_count_y*tile_side_in_meters,
                                           0.9f*tile_depth_in_meters);
        game_state->stair_collision =
            make_simple_grounded_collision(game_state,
                                           tile_side_in_meters,
                                           2.0f*tile_side_in_meters,
                                           1.1f*tile_depth_in_meters);
        game_state->sword_collision =
            make_simple_grounded_collision(game_state,
                                           tile_side_in_meters,
                                           tile_side_in_meters,
                                           0.1f);
        game_state->player_collision =
            make_simple_grounded_collision(game_state, 1.0f, 0.5f, 1.0f);
        game_state->monster_collision =
            make_simple_grounded_collision(game_state,
                                           0.8f*tile_side_in_meters,
                                           0.5f*tile_side_in_meters,
                                           tile_side_in_meters);
        game_state->familiar_collision =
            make_simple_grounded_collision(game_state,
                                           0.5f*tile_side_in_meters,
                                           0.5f*tile_side_in_meters,
                                           0.5f*tile_side_in_meters);
        
        game_state->wall_sprite       = debug_load_bitmap(memory->debug_os_read_file, thread, "../res/wall.bmp");
        game_state->stairwell_sprite  = debug_load_bitmap(memory->debug_os_read_file, thread, "../res/stair.bmp");
        game_state->grass_sprites[0]  = debug_load_bitmap(memory->debug_os_read_file, thread, "../res/grass00.bmp");
        game_state->grass_sprites[1]  = debug_load_bitmap(memory->debug_os_read_file, thread, "../res/grass01.bmp");
        game_state->stone_sprites[0]  = debug_load_bitmap(memory->debug_os_read_file, thread, "../res/stone00.bmp");
        game_state->stone_sprites[1]  = debug_load_bitmap(memory->debug_os_read_file, thread, "../res/stone01.bmp");
        game_state->tuft_sprites[0]   = debug_load_bitmap(memory->debug_os_read_file, thread, "../res/tuft0.bmp");
        game_state->tuft_sprites[1]   = debug_load_bitmap(memory->debug_os_read_file, thread, "../res/tuft1.bmp");
        game_state->shadow_sprite     = debug_load_bitmap(memory->debug_os_read_file, thread, "../res/shadow.bmp");
        game_state->player_sprites[0] = debug_load_bitmap(memory->debug_os_read_file, thread, "../res/skull_back.bmp");
        game_state->player_sprites[1] = debug_load_bitmap(memory->debug_os_read_file, thread, "../res/skull_right.bmp");
        game_state->player_sprites[2] = debug_load_bitmap(memory->debug_os_read_file, thread, "../res/skull_front.bmp");
        game_state->player_sprites[3] = debug_load_bitmap(memory->debug_os_read_file, thread, "../res/skull_left.bmp");
        game_state->bat_sprite        = debug_load_bitmap(memory->debug_os_read_file, thread, "../res/bat.bmp");
        game_state->sword_sprite      = debug_load_bitmap(memory->debug_os_read_file, thread, "../res/shadow.bmp");
        
        add_low_entity(game_state, EntityType_Null, null_position());

        random_series_t series = random_seed(0);
       
        u32 screen_base_x = 0;
        u32 screen_base_y = 0;
        u32 screen_base_z = 0;

        u32 screen_x = screen_base_x;
        u32 screen_y = screen_base_y;
        u32 abs_tile_z = screen_base_z;
        
        // TODO(xkazu0x): replace all this with real world generation
        b32 door_left = false;
        b32 door_right = false;
        b32 door_top = false;
        b32 door_bottom = false;
        b32 door_up = false;
        b32 door_down = false;
        
        for (u32 screen_index = 0;
             screen_index < 100;
             ++screen_index)
        {
#if 1
            u32 door_direction = random_choice(&series, (door_up || door_down) ? 2 : 4);
#else
            u32 door_direction = random_choice(&series, 2);
#endif

            //door_direction = 3;
            
            b32 created_z_door = false;
            if (door_direction == 3) {
                created_z_door = true;
                door_down = true;
            } else if (door_direction == 2) {
                created_z_door = true;
                door_up = true;
                // if (abs_tile_z == screen_base_z) {
                //     door_up = true;
                // } else {
                //     door_down = true;
                // }
            } else if (door_direction == 1) {
                door_right = true;
            } else {
                door_top = true;
            }

            add_standard_room(game_state, 
                              screen_x*tile_count_x + tile_count_x/2,
                              screen_y*tile_count_y + tile_count_y/2,
                              abs_tile_z);
            
            for (u32 tile_y = 0; tile_y < tile_count_y; ++tile_y) {
                for (u32 tile_x = 0; tile_x < tile_count_x; ++tile_x) {
                    u32 abs_tile_x = screen_x*tile_count_x + tile_x;
                    u32 abs_tile_y = screen_y*tile_count_y + tile_y;
                    
                    b32 should_be_door = false;
                    if ((tile_x == 0) && (!door_left || (tile_y != (tile_count_y/2)))) {
                        should_be_door = true;
                    }
                    
                    if ((tile_x == (tile_count_x - 1)) && (!door_right || (tile_y != (tile_count_y/2)))) {
                        should_be_door = true;
                    }
                    
                    if ((tile_y == 0) && (!door_bottom || (tile_x != (tile_count_x/2)))) {
                        should_be_door = true;
                    }
                    
                    if ((tile_y == (tile_count_y - 1)) && (!door_top || (tile_x != (tile_count_x/2)))) {
                        should_be_door = true;
                    }
                    
                    if (should_be_door) {
                        add_wall(game_state, abs_tile_x, abs_tile_y, abs_tile_z);
                    } else if (created_z_door) {
                        if (((abs_tile_z % 2) && (tile_x == 10) && (tile_y == 5)) ||
                            (!(abs_tile_z % 2) && (tile_x == 4) && (tile_y == 5))) {
                            add_stair(game_state, abs_tile_x, abs_tile_y, door_down ? abs_tile_z - 1 : abs_tile_z);
                        }
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

            if (door_direction == 3) {
                abs_tile_z -= 1;
            } else if (door_direction == 2) {
                abs_tile_z += 1;
                // if (abs_tile_z == screen_base_z) {
                //     abs_tile_z = screen_base_z + 1;
                // } else {
                //     abs_tile_z = screen_base_z;
                // }
            } else if (door_direction == 1) {
                screen_x += 1;
            } else {
                screen_y += 1;
            }
        }

        u32 camera_tile_x = screen_base_x*tile_count_x + tile_count_x/2;
        u32 camera_tile_y = screen_base_y*tile_count_y + tile_count_y/2;
        u32 camera_tile_z = screen_base_z;
        
        World_Position new_camera_pos = {};
        new_camera_pos = chunk_position_from_tile_position(game_state->world, camera_tile_x, camera_tile_y, camera_tile_z);
        game_state->camera_pos = new_camera_pos;
        
        add_monster(game_state, camera_tile_x - 4, camera_tile_y + 2, camera_tile_z);
        for (u32 familiar_index = 0;
             familiar_index < 1;
             ++familiar_index) {
            s32 familiar_offset_x = random_between(&series, -5, 5);
            s32 familiar_offset_y = random_between(&series, -3, 3);
            if ((familiar_offset_x != 0) ||
                (familiar_offset_y != 0)) {
                //add_familiar(game_state, camera_tile_x + familiar_offset_x, camera_tile_y + familiar_offset_y, camera_tile_z);
            }
        }
            
        memory->initialized = true;
    }

    //
    // NOTE(xkazu0x): initialize transient state
    //
    assert(sizeof(Transient_State) <= memory->transient_storage_size);
    Transient_State *tran_state = (Transient_State *)memory->transient_storage;
    if (!tran_state->initialized) {
        tran_state->arena = make_arena(memory->transient_storage_size - sizeof(Transient_State),
                                       (u8 *)memory->transient_storage + sizeof(Transient_State));
        
        tran_state->ground_buffer_count = 64;
        tran_state->ground_buffers = push_array(&tran_state->arena,
                                                Ground_Buffer,
                                                tran_state->ground_buffer_count);
        
        for (u32 ground_buffer_index = 0;
             ground_buffer_index < tran_state->ground_buffer_count;
             ++ground_buffer_index) {
            Ground_Buffer *ground_buffer = tran_state->ground_buffers + ground_buffer_index;
            ground_buffer->bitmap = make_empty_bitmap(&tran_state->arena, ground_buffer_width, ground_buffer_height, false);
            ground_buffer->position = null_position();
        }

        game_state->test_diffuse = make_empty_bitmap(&tran_state->arena, 256, 256, false);
        draw_rect(&game_state->test_diffuse, make_vec2(0.0f), make_vec2((f32)game_state->test_diffuse.width, (f32)game_state->test_diffuse.height), make_vec4(0.5f, 0.5f, 0.5f, 1.0f));
        
        game_state->test_normal = make_empty_bitmap(&tran_state->arena, game_state->test_diffuse.width, game_state->test_diffuse.height, false);
        make_sphere_normal_map(&game_state->test_normal, 0.0f);
        make_sphere_diffuse_map(&game_state->test_diffuse);
        //make_sphere_normal_map(&game_state->test_normal, 0.0f, 0.0f, 1.0f);
        //make_pyramid_normal_map(&game_state->test_normal, 0.0f);

        tran_state->env_map_width = 512;
        tran_state->env_map_height = 256;
        for (u32 map_index = 0;
             map_index < array_count(tran_state->env_maps);
             ++map_index) {
            Environment_Map *map = tran_state->env_maps + map_index;
            u32 width = tran_state->env_map_width;
            u32 height = tran_state->env_map_height;
            for (u32 lod_index = 0;
                 lod_index < array_count(map->lod);
                 ++lod_index) {
                
                map->lod[lod_index] = make_empty_bitmap(&tran_state->arena, width, height, false);
                width >>= 1;
                height >>= 1;
            }
        }
        
        tran_state->initialized = true;
    }

#if 0
    // NOTE(xkazu0x): regen ground buffers when game code reloaded
    if (input->executable_reloaded) {
        for (u32 ground_buffer_index = 0;
             ground_buffer_index < tran_state->ground_buffer_count;
             ++ground_buffer_index) {
            Ground_Buffer *ground_buffer = tran_state->ground_buffers + ground_buffer_index;
            ground_buffer->position = null_position();
        }
    }
#endif
    
    World *world = game_state->world;

    // NOTE(xkazu0x): gamepad control
    for (u32 gamepad_index = 0;
         gamepad_index < array_count(input->gamepads);
         ++gamepad_index) {
        Gamepad *gamepad = get_gamepad(input, gamepad_index);
        Controlled_Player *controlled_player = game_state->controlled_players + gamepad_index;
        if (controlled_player->entity_index == 0) {
            if ((gamepad->start.pressed) ||
                ((gamepad_index == 1) && (input->keyboard[Key_Space].pressed))) {
                *controlled_player = {};
                controlled_player->entity_index = add_player(game_state).low_index;
            }
        } else {
            controlled_player->dd_pos = {};
            controlled_player->d_z = 0.0f;
            controlled_player->d_sword = {};
            
            controlled_player->dd_pos = make_vec2(gamepad->left_stick.x, gamepad->left_stick.y);

            // NOTE(xkazu0x): combine keyboard and gamepad inputs
            // only for the first player
            if (gamepad_index == 1) {
                if (input->keyboard[Key_W].down) controlled_player->dd_pos.y = 1.0f;
                if (input->keyboard[Key_S].down) controlled_player->dd_pos.y = -1.0f;
                if (input->keyboard[Key_A].down) controlled_player->dd_pos.x = -1.0f;
                if (input->keyboard[Key_D].down) controlled_player->dd_pos.x = 1.0f;
                if (input->keyboard[Key_Space].pressed) controlled_player->d_z = 3.0f;

                if (input->keyboard[Key_Up].pressed) controlled_player->d_sword = make_vec2(0.0f, 1.0f);
                if (input->keyboard[Key_Down].pressed) controlled_player->d_sword = make_vec2(0.0f, -1.0f);
                if (input->keyboard[Key_Left].pressed) controlled_player->d_sword = make_vec2(-1.0f, 0.0f);
                if (input->keyboard[Key_Right].pressed) controlled_player->d_sword = make_vec2(1.0f, 0.0f);
            }
        }
    }

    //
    // NOTE(xkazu0x): Update and Render
    //
    Bitmap _draw_buffer = {};
    Bitmap *draw_buffer = &_draw_buffer;
    draw_buffer->width  = framebuffer->width;
    draw_buffer->height = framebuffer->height;
    draw_buffer->pitch  = framebuffer->pitch;
    draw_buffer->memory = framebuffer->memory;
    
    // NOTE(xkazu0x): initialize renderer memory
    Temp_Memory render_memory = begin_temp_memory(&tran_state->arena);
    // TODO(xkazu0x): Decide what out push buffer size is!
    Render_Group *render_group = render_group_alloc(&tran_state->arena, MB(4), draw_buffer->width, draw_buffer->height);

    render_clear(render_group, make_vec4(0.2f, 0.3f, 0.3f, 1.0f));

#if 0
    for (u32 y = 0; y < 12; ++y) {
        for (u32 x = 0; x < 20; ++x) {
            Vec3 color = make_rgb(57.0f, 44.0f, 49.0f);
            
            if ((x + y) % 2 == 0) {
                color = make_rgb(74.0f, 60.0f, 74.0f);
            }
            
            Vec2 min = make_vec2((f32)x*tile_size_in_pixels, (f32)y*tile_size_in_pixels);
            Vec2 max = make_vec2(min.x + tile_size_in_pixels, min.y + tile_size_in_pixels);
            
            draw_rect(draw_buffer, min, max, color);
        }
    }
#endif

    Vec2 screen_center = 0.5f*make_vec2((f32)draw_buffer->width, (f32)draw_buffer->height);
    
    Rect2 screen_bounds = get_camera_rect_at_target(render_group);
    Rect3 camera_bounds_in_meters = make_rect3_min_max(make_vec3(screen_bounds.min, 0.0f), make_vec3(screen_bounds.max, 0.0f));
    camera_bounds_in_meters.min.z = -3.0f*game_state->typical_floor_height;
    camera_bounds_in_meters.max.z = 2.0f*game_state->typical_floor_height;
    
    // NOTE(xkazu0x): Ground chunk rendering
    for (u32 ground_buffer_index = 0;
         ground_buffer_index < tran_state->ground_buffer_count;
         ++ground_buffer_index) {
        Ground_Buffer *ground_buffer = tran_state->ground_buffers + ground_buffer_index;
        if (is_valid(ground_buffer->position)) {
            Bitmap *bitmap = &ground_buffer->bitmap;
            
            Vec3 delta = subtract(game_state->world, &ground_buffer->position, &game_state->camera_pos);
            if ((delta.z >= -1.0f) && (delta.z < 1.0f)) {
                    Render_Basis *basis = push_struct(&tran_state->arena, Render_Basis);
                    render_group->default_basis = basis;
                    basis->pos = delta;

                    f32 ground_side_in_meters = world->chunk_dim_in_meters.x;
            
                    render_bitmap(render_group, bitmap, make_vec3(0.0f), ground_side_in_meters);
#if 1
                    render_rect_outline(render_group,
                                        make_vec3(0.0f),
                                        make_vec2(ground_side_in_meters),
                                        make_vec4(1.0f, 1.0f, 0.0f, 1.0f));
#endif
            }
        }
    }
    
    {
        // NOTE(xkazu0x): Ground chunk updating
        World_Position min_chunk_pos = map_into_chunk_space(world, game_state->camera_pos, get_rect_min(camera_bounds_in_meters));
        World_Position max_chunk_pos = map_into_chunk_space(world, game_state->camera_pos, get_rect_max(camera_bounds_in_meters));
    
        for (s32 chunk_z = min_chunk_pos.chunk_z;
             chunk_z <= max_chunk_pos.chunk_z;
             ++chunk_z) {
            for (s32 chunk_y = min_chunk_pos.chunk_y;
                 chunk_y <= max_chunk_pos.chunk_y;
                 ++chunk_y) {
                for (s32 chunk_x = min_chunk_pos.chunk_x;
                     chunk_x <= max_chunk_pos.chunk_x;
                     ++chunk_x) {
                    World_Position chunk_center_pos = centered_chunk_point(chunk_x, chunk_y, chunk_z);
                    Vec3 chunk_rel_pos = subtract(world, &chunk_center_pos, &game_state->camera_pos);
                    
                    f32 furthest_ground_buffer_length_squared = 0.0f;
                    Ground_Buffer *furthest_ground_buffer = 0;
                        
                    // TODO(xkazu0x): This is super inefficient!
                    for (u32 ground_buffer_index = 0;
                         ground_buffer_index < tran_state->ground_buffer_count;
                         ++ground_buffer_index) {
                        Ground_Buffer *ground_buffer = tran_state->ground_buffers + ground_buffer_index;
                        if (are_in_same_chunk(world, &ground_buffer->position, &chunk_center_pos)) {
                            furthest_ground_buffer = 0;
                            break;
                        } else if (is_valid(ground_buffer->position)) {
                            Vec3 ground_rel_pos = subtract(world, &ground_buffer->position, &game_state->camera_pos);
                            f32 distance_length_squared = length_squared(ground_rel_pos.xy);
                            if (furthest_ground_buffer_length_squared < distance_length_squared) {
                                furthest_ground_buffer_length_squared = distance_length_squared;
                                furthest_ground_buffer = ground_buffer;
                            }
                        } else {
                            furthest_ground_buffer_length_squared = f32_max;
                            furthest_ground_buffer = ground_buffer;
                        }
                    }

                    if (furthest_ground_buffer) {
                        fill_ground_chunk(tran_state, game_state, furthest_ground_buffer, &chunk_center_pos);
                    }
                }
            }
        }
    }
    
    // NOTE(xkazu0x): initialize simulation memory
    // TODO(xkazu0x): How big do we actually want to expand here?
    // TODO(xkazu0x): Do we want to simulate upper floors?
    Vec3 sim_bounds_expansion = make_vec3(15.0f, 15.0f, 0.0f);
    Rect3 sim_bounds = rect_add_radius(camera_bounds_in_meters, sim_bounds_expansion);
    
    Temp_Memory sim_memory = begin_temp_memory(&tran_state->arena);
    World_Position sim_center_pos = game_state->camera_pos;
    Sim_Region *sim_region = begin_sim(&tran_state->arena, game_state, world, sim_center_pos, sim_bounds, clock->dt);

    Render_Basis *basis = push_struct(&tran_state->arena, Render_Basis);
    basis->pos = make_vec3(0.0f);
    render_group->default_basis = basis;
    
    render_rect_outline(render_group, make_vec3(0.0f), get_rect_dim(screen_bounds), make_vec4(0.0f, 1.0f, 0.0f, 1.0f));
    //render_rect_outline(render_group, make_vec3(0.0f), get_rect_dim(camera_bounds_in_meters).xy, make_vec4(1.0f, 0.0f, 0.0f, 1.0f));
    render_rect_outline(render_group, make_vec3(0.0f), get_rect_dim(sim_bounds).xy, make_vec4(0.0f, 1.0f, 1.0f, 1.0f));
    render_rect_outline(render_group, make_vec3(0.0f), get_rect_dim(sim_region->bounds).xy, make_vec4(1.0f, 0.0f, 1.0f, 1.0f));

    Vec3 camera_pos = subtract(world, &game_state->camera_pos, &sim_center_pos);
    
    // NOTE(xkazu0x): entity type handling
    // TODO(xkazu0x): move this out into excalibur_entity.cpp
    for (u32 entity_index = 0;
         entity_index < sim_region->entity_count;
         ++entity_index) {
        Sim_Entity *entity = sim_region->entities + entity_index;
        if (entity->updatable) {
            f32 dt = clock->dt;

            f32 shadow_alpha = 1.0f - 0.5f*entity->pos.z;
            if (shadow_alpha < 0.0f) shadow_alpha = 0.0f;

            Move_Spec move_spec = default_move_spec();
            Vec3 dd_pos = make_vec3(0.0f);

            Render_Basis *basis = push_struct(&tran_state->arena, Render_Basis);
            render_group->default_basis = basis;

            f32 fade_top_end_z = 0.75f*game_state->typical_floor_height;
            f32 fade_top_start_z = 0.5f*game_state->typical_floor_height;
            f32 fade_bottom_start_z = -2.0f*game_state->typical_floor_height;
            f32 fade_bottom_end_z = -2.25f*game_state->typical_floor_height;

            render_group->global_alpha = 1.0f;
            Vec3 camera_relative_ground_point = get_entity_ground_point(entity) - camera_pos;
            if (camera_relative_ground_point.z > fade_top_start_z) {
                render_group->global_alpha = clamp01_map_to_range(fade_top_end_z, camera_relative_ground_point.z, fade_top_start_z);
            } else if (camera_relative_ground_point.z < fade_bottom_start_z) {
                render_group->global_alpha = clamp01_map_to_range(fade_bottom_end_z, camera_relative_ground_point.z, fade_bottom_start_z);
            }
            
            switch (entity->type) {
                case EntityType_Space: {
#if 1
                    for (u32 volume_index = 0;
                         volume_index < entity->collision->volume_count;
                         ++volume_index) {
                        Sim_Entity_Collision_Volume *volume = entity->collision->volumes + volume_index;
                        render_rect_outline(render_group, volume->offset - make_vec3(0.0f, 0.0f, 0.5f*volume->dim.z), volume->dim.xy, make_vec4(0.0f, 1.0f, 1.0f, 1.0f));
                    }
#endif
                } break;
                    
                case EntityType_Wall: {
                    Vec2 dim = entity->collision->total_volume.dim.xy;
                    render_rect(render_group, make_vec3(0.0f), dim, make_vec4(1.0f, 0.5f, 0.2f, 1.0f));
                    
                    Bitmap *sprite = &game_state->wall_sprite;
                    
                    render_bitmap(render_group, sprite, make_vec3(0.0f), dim.y);
                } break;
                    
                case EntityType_Stairwell: {
                    render_rect(render_group, make_vec3(0.0f), entity->walkable_dim, make_vec4(1.0f, 0.0f, 1.0f, 1.0f));
                    //render_rect(render_group, make_vec3(0.0f, entity->walkable_height, 0.0f), entity->walkable_dim, make_vec4(0.0f, 1.0f, 1.0f, 1.0f));
                    
                    Bitmap *sprite = &game_state->stairwell_sprite;
                    
                    render_bitmap(render_group, sprite, make_vec3(0.0f), 3.0f);
                    //render_bitmap(render_group, sprite, make_vec3(0.0f, 0.0f, entity->walkable_height));
                } break;

                case EntityType_Player: {
                    for (u32 controlled_index = 0;
                         controlled_index < array_count(game_state->controlled_players);
                         ++controlled_index) {
                        Controlled_Player *controlled_player = game_state->controlled_players + controlled_index;
                        if (entity->storage_index == controlled_player->entity_index) {
                            if (controlled_player->d_z != 0.0f) {
                                entity->d_pos.z = controlled_player->d_z;
                            }

                            move_spec.unit_max_accel_vector = true;
                            move_spec.speed = 50.0f;
                            move_spec.drag = 5.0f;
                            dd_pos = make_vec3(controlled_player->dd_pos, 0.0f);
                            
                            if ((controlled_player->d_sword.x != 0.0f) || (controlled_player->d_sword.y != 0.0f)) {
                                Sim_Entity *sword = entity->sword.ptr;
                                if (sword && is_entity_flag_set(sword, EntityFlag_NonSpatial)) {
                                    sword->distance_limit = 3.0f;
                                    make_entity_spatial(sword, entity->pos, 10.0f*make_vec3(controlled_player->d_sword, 0.0f));
                                    add_collision_rule(game_state, sword->storage_index, entity->storage_index, false);
                                }
                            }
                        }
                    }
                    
                    render_rect(render_group, make_vec3(0.0f), entity->collision->total_volume.dim.xy, make_vec4(1.0f, 0.5f, 0.2f, 1.0f));

                    Bitmap *shadow_sprite = &game_state->shadow_sprite;
                    Bitmap *player_sprite = &game_state->player_sprites[entity->direction];
                    
                    render_bitmap(render_group, shadow_sprite, make_vec3(0.0f), 1.0f, make_vec4(1.0f, 1.0f, 1.0f, shadow_alpha));
                    render_bitmap(render_group, player_sprite, make_vec3(0.0f), 1.0f);
                    
                    draw_hit_points(render_group, entity);
                } break;
                    
                case EntityType_Sword: {
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
                        clear_collision_rules_for(game_state, entity->storage_index);
                        make_entity_non_spatial(entity);
                    }

                    render_rect(render_group, make_vec3(0.0f), entity->collision->total_volume.dim.xy, make_vec4(0.0f, 1.0f, 0.0f, 1.0f));

                    Bitmap *sprite = &game_state->sword_sprite;
                    set_bitmap_align(sprite, 0.5f*make_vec2((f32)sprite->width, (f32)sprite->height));
                    
                    render_bitmap(render_group, sprite, make_vec3(0.0f), 1.0f);
                } break;
                    
                case EntityType_Familiar: {
                    Sim_Entity *closest_player = 0;
                    f32 closest_player_distance_sqr = square(10.0f); // NOTE(xkazu0x): ten meter maximun search!

#if 0
                    // TODO(xkazu0x): make spatial queries easy for things!
                    Sim_Entity *test_entity = sim_region->entities;
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
#endif
                    
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
                
                    entity->t_bob += dt;
                    if (entity->t_bob > (2.0f*pi32)) {
                        entity->t_bob -= (2.0f*pi32);
                    }
                    
                    f32 bob_sin = 0.3f*sin_f32(5.0f*entity->t_bob);
                    
                    render_rect(render_group, make_vec3(0.0f), entity->collision->total_volume.dim.xy, make_vec4(0.0f, 1.0f, 0.0f, 1.0f));

                    Bitmap *shadow_sprite = &game_state->shadow_sprite;
                    Bitmap *bat_sprite = &game_state->bat_sprite;

                    set_bitmap_align(shadow_sprite, 0.5f*make_vec2((f32)shadow_sprite->width, (f32)shadow_sprite->height));
                    set_bitmap_align(bat_sprite, 0.5f*make_vec2((f32)bat_sprite->width, (f32)bat_sprite->height));

                    render_bitmap(render_group, shadow_sprite, make_vec3(0.0f), 1.0f, make_vec4(1.0f, 1.0f, 1.0f, (0.5f*shadow_alpha) - bob_sin));
                    render_bitmap(render_group, bat_sprite, make_vec3(0.0f, 0.0f, bob_sin + 0.4f), 1.0f);
                } break;
                    
                case EntityType_Monster: {
                    render_rect(render_group, make_vec3(0.0f), entity->collision->total_volume.dim.xy, make_vec4(1.0f, 0.0f, 0.0f, 1.0f));
                    
                    Bitmap *shadow_sprite = &game_state->shadow_sprite;
                    Bitmap *monster_sprite = &game_state->player_sprites[2];

                    set_bitmap_align(shadow_sprite, 0.5f*make_vec2((f32)shadow_sprite->width, (f32)shadow_sprite->height));
                    set_bitmap_align(monster_sprite, 0.5f*make_vec2((f32)monster_sprite->width, (f32)monster_sprite->height));
                    
                    render_bitmap(render_group, shadow_sprite, make_vec3(0.0f), 1.0f, make_vec4(1.0f, 1.0f, 1.0f, shadow_alpha));
                    render_bitmap(render_group, monster_sprite, make_vec3(0.0f), 1.0f);
                    
                    draw_hit_points(render_group, entity);
                } break;
                {
                    invalid_default_case;
                }
            }

            if (!is_entity_flag_set(entity, EntityFlag_NonSpatial) &&
                is_entity_flag_set(entity, EntityFlag_Moveable)) {
                move_entity(game_state, sim_region, entity, clock->dt, &move_spec, dd_pos);
            }

            basis->pos = get_entity_ground_point(entity);
        }
    }

#if 0
    game_state->time += clock->dt;
    f32 axis_scale = 50.0f;
    f32 angle = game_state->time;
    
#if 1
    Vec2 disp = make_vec2(40.0f*cos_f32(angle), 20.0f*sin_f32(2.0f*angle));
#else
    Vec2 disp = make_vec2(0.0f);
#endif
    
    // TODO(xkazu0x): Add a perpendicular operator!
    Vec2 origin = screen_center;
    
#if 1
    Vec2 axis_x = axis_scale*make_vec2(cos_f32(5.0f*angle), sin_f32(5.0f*angle));
    Vec2 axis_y = perp(axis_x);
#else
    Vec2 axis_x = axis_scale*make_vec2(1.0f, 0.0f);
    Vec2 axis_y = axis_scale*make_vec2(0.0f, 1.0f);
#endif
    
#if 0
    Vec4 color = make_vec4(0.5f + 0.5f*sin_f32(angle),
                           0.5f + 0.5f*cos_f32(3.0f*angle),
                           0.5f + 0.5f*sin_f32(6.0f*angle),
                           0.5f + 0.5f*cos_f32(9.0f*angle));
#else
    Vec4 color = make_vec4(1.0f);
#endif
    
    render_coordinate_system(render_group,
                             disp + origin - 0.5f*axis_x - 0.5f*axis_y, axis_x, axis_y, color,
                             &game_state->test_diffuse,
                             &game_state->test_normal,
                             tran_state->env_maps + 2,
                             tran_state->env_maps + 1,
                             tran_state->env_maps + 0);

    Vec3 map_color[] = {
        make_vec3(1.0f, 0.0f, 0.0f),
        make_vec3(0.0f, 1.0f, 0.0f),
        make_vec3(0.0f, 0.0f, 1.0f),
    };
    for (u32 map_index = 0;
         map_index < array_count(tran_state->env_maps);
         ++map_index) {
        Environment_Map *map = tran_state->env_maps + map_index;
        Bitmap *lod = map->lod + 0;

        s32 checker_width = 16;
        s32 checker_height = 16;
        
        b32 row_checker_on = false;
        for (s32 y = 0;
             y < lod->height;
             y += checker_height) {
            b32 checker_on = row_checker_on;
            for (s32 x = 0;
                 x < lod->width;
                 x += checker_width) {
                Vec4 color = checker_on ? make_vec4(map_color[map_index], 1.0f) : make_vec4(0.0f, 0.0f, 0.0f, 1.0f);
                Vec2 min = make_vec2((f32)x, (f32)y);
                Vec2 max = min + make_vec2((f32)checker_width, (f32)checker_height);
                draw_rect(lod, min, max, color);
                checker_on = !checker_on;
            }
            row_checker_on = !row_checker_on;
        }
    }    
    tran_state->env_maps[0].pos_z = -2.5f;
    tran_state->env_maps[1].pos_z = 0.0f;
    tran_state->env_maps[2].pos_z = 2.5f;
    
    Vec2 map_pos = make_vec2(0.0f);
    for (u32 map_index = 0;
         map_index < array_count(tran_state->env_maps);
         ++map_index) {
        Environment_Map *map = tran_state->env_maps + map_index;
        Bitmap *lod = map->lod + 0;
        
        axis_x = 0.2f*make_vec2((f32)lod->width, 0.0f);
        axis_y = 0.2f*make_vec2(0.0f, (f32)lod->height);
        
        render_coordinate_system(render_group, map_pos, axis_x, axis_y, make_vec4(1.0f), lod, 0, 0, 0, 0);
        map_pos += axis_y + make_vec2(0.0f, 4.0f);
    }
#endif

    // NOTE(xkazu0x): draw to screen and end simulation
    render_group_draw(render_group, draw_buffer);
    end_sim(sim_region, game_state);
    
    // NOTE(xkazu0x): reset and check renderer and simulation memory
    end_temp_memory(&sim_memory);
    end_temp_memory(&render_memory);
    
    check_arena(&game_state->world_arena);
    check_arena(&tran_state->arena);
}

#if OS_WINDOWS
#include <windows.h>
int WINAPI
WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    return(0);
}
#endif
