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

struct canonical_position {
    vec2i tile_map_index;
    vec2i tile_index;
    vec2f tile_rel; // NOTE(xkazu0x): tile relative point
};

struct tile_map {
    u32 *tiles;
};

struct world_map {
    f32 tile_size_in_meters;
    s32 tile_size_in_pixels;
    f32 meters_to_pixels;
    
    vec2i tile_count;
    vec2f offset;
    
    vec2i tile_map_count;
    tile_map *tile_maps;
};

struct game_state {
    canonical_position player_pos;
};

#endif // EXCALIBUR_H
