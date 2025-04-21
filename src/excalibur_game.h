#ifndef EXCALIBUR_GAME_H
#define EXCALIBUR_GAME_H

// NOTE(xkazu0x):
// EXCALIBUR_INTERNAL:
// > 0 - build for public release
// > 1 - build for develop only
// EXCALIBUR_DEBUG:
// > 0 - not slow code allowed
// > 1 - slow code welcome

struct memory_arena_t {
    memi size;
    memi used;
    u8 *base;
};

internal memory_arena_t
memory_arena_create(memi size, u8 *base) {
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

struct bitmap_t {
    s32 width;
    s32 height;
    u32 *pixels;
};

enum entity_type_t {
    ENTITY_TYPE_NULL,
    ENTITY_TYPE_WALL,
    ENTITY_TYPE_PLAYER,
    ENTITY_TYPE_FAMILIAR,
    ENTITY_TYPE_MONSTER,
    ENTITY_TYPE_SWORD,
};

#define HIT_POINT_FILLED_MAX 4
struct hit_point_t {
    u8 flags;
    u8 filled_amount;
};

struct low_entity_t {
    entity_type_t type;
    
    world_position_t pos;
    f32 width;
    f32 height;
    
    s32 d_tile_z; // note(xkazu0x): this is for "stairs"
    b32 collides;

    u32 high_entity_index;

    // TODO(xkazu0x): should hit point be entities?
    u32 hit_point_max;
    hit_point_t hit_points[16];

    u32 sword_low_index;
    f32 distance_remaining;
};

struct high_entity_t {
    vec2f pos; // note(xkazu0x): relative to the camera
    vec2f d_pos;
    u32 chunk_z;
    u32 direction;

    f32 t_bob;
    f32 z;
    f32 dz;

    u32 low_entity_index;
};

struct entity_t {
    u32 low_index;
    low_entity_t *low;
    high_entity_t *high;
};

struct entity_visible_piece_t {
    bitmap_t *bitmap;
    vec3f offset;
    
    vec4f color;
    vec2f size;
    
    f32 entity_zc;
};

struct game_state_t {
    memory_arena_t world_arena;
    world_t *world;

    u32 camera_following_entity_index;
    world_position_t camera_pos;

    u32 low_entity_count;
    low_entity_t low_entities[100000];

    u32 high_entity_count;
    high_entity_t high_entities_[256];
    
    u32 player_gamepad_index[GAMEPAD_COUNT_MAX];
    bitmap_t player_sprites[4];
    bitmap_t wall_sprite;
    bitmap_t bat_sprite;
    bitmap_t shadow_sprite;
    bitmap_t sword_sprite;

    f32 meters_to_pixels;
};

// TODO(xkazu0x): this is dumb, this should just be part of
// the renderer pushbuffer - add correction of coordinates
// in there and be done with it.
struct entity_visible_piece_group_t {
    game_state_t *game_state;
    u32 piece_count;
    entity_visible_piece_t pieces[32];
};

#endif // EXCALIBUR_GAME_H
