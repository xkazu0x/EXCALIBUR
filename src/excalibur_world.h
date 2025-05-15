#ifndef EXCALIBUR_WORLD_H
#define EXCALIBUR_WORLD_H

struct World_Position {
    s32 chunk_x;
    s32 chunk_y;
    s32 chunk_z;
    
    // NOTE(xkazu0x): offset from the chunk center
    Vec3 offset_; 
};

struct World_Entity_Block {
    u32 entity_count;
    u32 low_entity_index[16];
    World_Entity_Block *next;
};

struct World_Chunk {
    s32 chunk_x;
    s32 chunk_y;
    s32 chunk_z;
    World_Entity_Block entity_block;
    World_Chunk *next_in_hash;
};

struct World {
    f32 tile_side_in_meters;
    f32 tile_depth_in_meters;
    Vec3 chunk_dim_in_meters;
    
    // NOTE(xkazu0x): at the moment, this needs to be a power of two!
    World_Chunk chunk_hash[4096];
    World_Entity_Block *first_free;
};

#endif // EXCALIBUR_WORLD_H
