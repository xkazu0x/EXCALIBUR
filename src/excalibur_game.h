#ifndef EXCALIBUR_GAME_H
#define EXCALIBUR_GAME_H

// NOTE(xkazu0x):
// EXCALIBUR_INTERNAL:
// > 0 - build for public release
// > 1 - build for develop only
// EXCALIBUR_DEBUG:
// > 0 - not slow code allowed
// > 1 - slow code welcome

#include "excalibur_tile.h"

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

struct game_state_t {
    tile_map_position_t player_pos;
    bitmap_t test_bitmap;
    
    memory_arena_t world_arena;
    world_t *world;
};

#endif // EXCALIBUR_GAME_H
