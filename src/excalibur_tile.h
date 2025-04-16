#ifndef EXCALIBUR_TILE_H
#define EXCALIBUR_TILE_H

struct tile_chunk_position_t {
    u32 tile_chunk_x;
    u32 tile_chunk_y;
    u32 tile_chunk_z;

    u32 tile_x;
    u32 tile_y;
};

struct tile_map_position_t {
    // NOTE(xkazu0x): fixed point tile location. the high
    // bits are the tile chunk index, and the low bits are
    // the tile index in the chunk.
    u32 tile_x;
    u32 tile_y;
    u32 tile_z;

    // NOTE(xkazu0x): offset from the tile center
    vec2f tile_offset_;
};

struct tile_chunk_t {
    u32 tile_chunk_x;
    u32 tile_chunk_y;
    u32 tile_chunk_z;
    u32 *tiles;
    tile_chunk_t *next_in_hash;
};

struct tile_map_t {
    u32 chunk_shift;
    u32 chunk_mask;
    u32 chunk_dim;
    
    f32 tile_size_in_meters;

    // NOTE(xkazu0x): at the moment, this needs to be a power of two!
    tile_chunk_t tile_chunk_hash[4096];
};

#endif // EXCALIBUR_TILE_H
