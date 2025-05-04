#ifndef EXCALIBUR_SIMULATION_H
#define EXCALIBUR_SIMULATION_H

struct move_spec_t {
    b32 unit_max_accel_vector;
    f32 speed;
    f32 drag;
};

enum entity_type_t {
    ENTITY_TYPE_NULL,
    
    ENTITY_TYPE_PLAYER,
    ENTITY_TYPE_WALL,
    ENTITY_TYPE_FAMILIAR,
    ENTITY_TYPE_MONSTER,
    ENTITY_TYPE_SWORD,
    ENTITY_TYPE_STAIRWELL,
};

#define HIT_POINT_FILLED_MAX 4
struct hit_point_t {
    u8 flags;
    u8 filled_amount;
};

struct sim_entity_t;
union entity_reference_t {
    sim_entity_t *ptr;
    u32 index;
};

enum sim_entity_flags_t {
    ENTITY_FLAG_COLLIDES    = (1 << 0),
    ENTITY_FLAG_NON_SPATIAL = (1 << 1),
    ENTITY_FLAG_MOVEABLE    = (1 << 2),
    ENTITY_FLAG_Z_SUPPORTED = (1 << 4),
    
    ENTITY_FLAG_SIMMING     = (1 << 30),
};

struct sim_entity_collision_volume_t {
    vec3 offset;
    vec3 dim;
};

struct sim_entity_collision_volume_group_t {
    sim_entity_collision_volume_t total_volume;

    // TODO(xkazu0x): volume_count is always expected to be greater than 0 if
    // the entity has any volume... in the future, this could be compressed if
    // necessary to say that the volume_count can be 0 if the total_value should
    // be used as only collision volume for the entity.
    u32 volume_count;
    sim_entity_collision_volume_t *volumes;
};

struct sim_entity_t {
    // NOTE(xkazu0x): only for the sim region
    world_chunk_t *old_chunk;
    u32 storage_index;
    b32 updatable;
    
    entity_type_t type;
    u32 flags;

    vec3 pos;
    vec3 d_pos;
    
    sim_entity_collision_volume_group_t *collision;
    
    u32 direction;
    f32 t_bob;
    
    s32 d_tile_z; // note(xkazu0x): this is for "stairs"

    // TODO(xkazu0x): should hit point be entities?
    u32 hit_point_max;
    hit_point_t hit_points[16];
    
    f32 distance_limit;
    entity_reference_t sword;

    // NOTE(xkazu0x): only for stairwells
    vec2 walkable_dim;
    f32 walkable_height;
};

struct sim_entity_hash_t {
    sim_entity_t *ptr;
    u32 index;
};

struct sim_region_t {
    // TODO(xkazu0x): need a hash table here to map stored entities indices
    // to sim entities
    
    world_t *world;
    f32 max_entity_radius;
    f32 max_entity_velocity;
    
    world_position_t origin;
    rect3 bounds;
    rect3 updatable_bounds;
    
    u32 max_entity_count;
    u32 entity_count;
    sim_entity_t *entities;

    f32 base_ground_z;
    
    // TODO(xkazu0x): do i really want a hash for this??
    // NOTE(xkazu0x): must be a power of two!
    sim_entity_hash_t hash[4096];
};

#endif // EXCALIBUR_SIMULATION_H
