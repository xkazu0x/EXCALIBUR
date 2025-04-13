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

struct high_entity_t {
    b32 exists;
    vec2f pos; // NOTE(xkazu0x): relative to the camera
    vec2f d_pos;
    u32 direction;
};

struct low_entity_t {
};

struct dormant_entity_t {
    tile_map_position_t pos;
    f32 width;
    f32 height;
};

enum entity_residence_t {
    ENTITY_RESIDENCE_UNDEFINED,
    ENTITY_RESIDENCE_DORMANT,
    ENTITY_RESIDENCE_LOW,
    ENTITY_RESIDENCE_HIGH,
};

struct entity_t {
    entity_residence_t residence;
    dormant_entity_t *dormant;
    low_entity_t *low;
    high_entity_t *high;
};

struct game_state_t {
    memory_arena_t world_arena;
    world_t *world;

    u32 camera_following_entity_index;
    tile_map_position_t camera_pos;

    u32 entity_count;
    entity_residence_t entity_residence[256];
    dormant_entity_t dormant_entities[256];
    low_entity_t low_entities[256];
    high_entity_t high_entities[256];
    
    u32 player_gamepad_index[GAMEPAD_COUNT_MAX];    
    bitmap_t player_sprites[4];
};

#endif // EXCALIBUR_GAME_H
