#ifndef EXCALIBUR_WORLD_H
#define EXCALIBUR_WORLD_H

struct world_position_t {
    s32 chunk_x;
    s32 chunk_y;
    s32 chunk_z;
    
    // NOTE(xkazu0x): offset from the chunk center
    Vec3 offset_; 
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
    f32 tile_side_in_meters;
    f32 tile_depth_in_meters;
    Vec3 chunk_dim_in_meters;
    
    // NOTE(xkazu0x): at the moment, this needs to be a power of two!
    world_chunk_t chunk_hash[4096];
    world_entity_block_t *first_free;
};

#endif // EXCALIBUR_WORLD_H
