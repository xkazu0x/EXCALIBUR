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

struct entity_t {
    b32 exists;
    tile_map_position_t pos;
    vec2f d_pos;
    u32 direction;
    f32 width;
    f32 height;
};

struct game_state_t {
    memory_arena_t world_arena;
    world_t *world;

    u32 camera_following_entity_index;
    tile_map_position_t camera_pos;

    u32 player_gamepad_index[GAMEPAD_COUNT_MAX];
    u32 entity_count;
    entity_t entities[256];
    
    bitmap_t player_sprites[4];
};

#endif // EXCALIBUR_GAME_H
