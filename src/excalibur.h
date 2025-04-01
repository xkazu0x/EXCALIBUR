#ifndef EXCALIBUR_H
#define EXCALIBUR_H

#include "excalibur_base.h"
#include "excalibur_math.h"
#include "excalibur_logger.h"

#include "excalibur_base.cpp"
#include "excalibur_math.cpp"
#include "excalibur_logger.cpp"

#include "excalibur_platform.h"

// NOTE(xkazu0x):
// EXCALIBUR_INTERNAL:
// > 0 - build for public release
// > 1 - build for develop only
// EXCALIBUR_DEBUG:
// > 0 - not slow code allowed
// > 1 - slow code welcome

struct tile_chunk_position {
    u32 tile_chunk_x;
    u32 tile_chunk_y;

    u32 tile_x;
    u32 tile_y;
};

struct tile_chunk {
    u32 *tiles;
};

struct world_position {
    u32 tile_x;
    u32 tile_y;

    vec2f tile_rel; // NOTE(xkazu0x): tile relative point
};

struct world_map {
    u32 chunk_shift;
    u32 chunk_mask;
    u32 chunk_dim;
    
    f32 tile_size_in_meters;
    s32 tile_size_in_pixels;
    f32 meters_to_pixels;
    
    s32 tile_chunk_count_x;
    s32 tile_chunk_count_y;
    tile_chunk *tile_chunks;
};

struct game_state {
    world_position player_pos;
};

#endif // EXCALIBUR_H
