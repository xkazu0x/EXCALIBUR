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

struct world_t {
    tile_map_t *tile_map;
};

struct bitmap_t {
    s32 width;
    s32 height;
    u32 *pixels;
};

enum entity_type_t {
    ENTITY_TYPE_NULL,
    ENTITY_TYPE_PLAYER,
    ENTITY_TYPE_WALL,
};

struct low_entity_t {
    entity_type_t type;
    tile_map_position_t pos;
    f32 width;
    f32 height;
    s32 d_tile_z; // NOTE(xkazu0x): this is for "stairs"
    b32 collides;

    u32 high_entity_index;
};

struct high_entity_t {
    vec2f pos; // NOTE(xkazu0x): relative to the camera
    vec2f d_pos;
    u32 tile_z;
    u32 direction;

    u32 low_entity_index;
};

struct entity_t {
    u32 low_index;
    low_entity_t *low;
    high_entity_t *high;
};

struct game_state_t {
    memory_arena_t world_arena;
    world_t *world;

    u32 camera_following_entity_index;
    tile_map_position_t camera_pos;

    u32 low_entity_count;
    low_entity_t low_entities[4096];

    u32 high_entity_count;
    high_entity_t high_entities_[256];
    
    u32 player_gamepad_index[GAMEPAD_COUNT_MAX];    
    bitmap_t player_sprites[4];
};

#endif // EXCALIBUR_GAME_H
