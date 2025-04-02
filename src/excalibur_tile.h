#ifndef EXCALIBUR_TILE_H
#define EXCALIBUR_TILE_H

struct tile_chunk_position_t {
    u32 tile_chunk_x;
    u32 tile_chunk_y;

    u32 tile_x;
    u32 tile_y;
};

struct tile_chunk_t {
    u32 *tiles;
};

struct tile_map_position_t {
    // NOTE(xkazu0x): fixed point tile location. the high
    // bits are the tile chunk index, and the low bits are
    // the tile index in the chunk.
    u32 tile_x;
    u32 tile_y;

    f32 tile_rel_x;
    f32 tile_rel_y;
};

struct tile_map_t {
    u32 chunk_shift;
    u32 chunk_mask;
    u32 chunk_dim;
    
    f32 tile_size_in_meters;
    s32 tile_size_in_pixels;
    f32 meters_to_pixels;
    
    u32 tile_chunk_count_x;
    u32 tile_chunk_count_y;
    tile_chunk_t *tile_chunks;
};

#endif // EXCALIBUR_TILE_H
