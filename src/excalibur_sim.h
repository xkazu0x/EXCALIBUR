#ifndef EXCALIBUR_SIM_H
#define EXCALIBUR_SIM_H

struct sim_entity_t {
    u32 storage_index;
    
    vec2f pos;
    u32 chunk_z;
    
    f32 z;
    f32 dz;
};

struct sim_region_t {
    // TODO(xkazu0x): need a hash table here to map stored entities indices
    // to sim entities
    
    world_t *world;
    world_position_t origin;
    rect2f bounds;
    
    u32 max_entity_count;
    u32 entity_count;
    sim_entity_t *entities;
};

#endif // EXCALIBUR_SIM_H
