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

struct world_t {
    tile_map_t *tile_map;
};

struct game_state_t {
    tile_map_position_t player_pos;

    memory_arena_t world_arena;
    world_t *world;
};

#endif // EXCALIBUR_GAME_H
