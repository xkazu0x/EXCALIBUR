#ifndef EXCALIBUR_WORLD_H
#define EXCALIBUR_WORLD_H

struct world_position_t {
    // NOTE(xkazu0x): fixed point tile location. the high
    // bits are the tile chunk index, and the low bits are
    // the tile index in the chunk.
    s32 tile_x;
    s32 tile_y;
    s32 tile_z;

    // NOTE(xkazu0x): offset from the tile center
    vec2f tile_offset_;
};

struct world_entity_block_t {
    u32 entity_count;
    u32 low_entity_index[16];
    world_entity_block_t *next;
};

struct world_chunk_t {
    s32 chunk_x;
    s32 chunk_y;
    s32 chunk_z;
    world_entity_block_t entity_block;
    world_chunk_t *next_in_hash;
};

struct world_t {
    f32 tile_size_in_meters;

    // NOTE(xkazu0x): at the moment, this needs to be a power of two!
    s32 chunk_shift;
    s32 chunk_mask;
    s32 chunk_dim;
    world_chunk_t chunk_hash[4096];
};

#endif // EXCALIBUR_WORLD_H
