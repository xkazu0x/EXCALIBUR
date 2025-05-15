#ifndef EXCALIBUR_SIMULATION_H
#define EXCALIBUR_SIMULATION_H

struct Move_Spec {
    b32 unit_max_accel_vector;
    f32 speed;
    f32 drag;
};

enum Entity_Type {
    EntityType_Null,
    EntityType_Space,
    EntityType_Player,
    EntityType_Wall,
    EntityType_Familiar,
    EntityType_Monster,
    EntityType_Sword,
    EntityType_Stairwell,
};

#define HIT_POINT_FILLED_MAX 4
struct Hit_Point {
    u8 flags;
    u8 filled_amount;
};

struct Sim_Entity;
union Entity_Reference {
    Sim_Entity *ptr;
    u32 index;
};

enum Sim_Entity_Flag {
    EntityFlag_Collides    = (1<<0),
    EntityFlag_NonSpatial  = (1<<1),
    EntityFlag_Moveable    = (1<<2),
    EntityFlag_ZSupported  = (1<<3),
    EntityFlag_Traversable = (1<<4),
   
    EntityFlag_Simming     = (1<<30),
};

struct Sim_Entity_Collision_Volume {
    Vec3 offset;
    Vec3 dim;
};

struct Sim_Entity_Collision_Volume_Group {
    Sim_Entity_Collision_Volume total_volume;

    // TODO(xkazu0x): volume_count is always expected to be greater than 0 if
    // the entity has any volume... in the future, this could be compressed if
    // necessary to say that the volume_count can be 0 if the total_value should
    // be used as only collision volume for the entity.
    u32 volume_count;
    Sim_Entity_Collision_Volume *volumes;
};

struct Sim_Entity {
    // NOTE(xkazu0x): only for the sim region
    World_Chunk *old_chunk;
    u32 storage_index;
    b32 updatable;
    
    Entity_Type type;
    u32 flags;

    Vec3 pos;
    Vec3 d_pos;
    
    Sim_Entity_Collision_Volume_Group *collision;
    
    u32 direction;
    f32 t_bob;
    
    s32 d_tile_z; // note(xkazu0x): this is for "stairs"

    // TODO(xkazu0x): should hit point be entities?
    u32 hit_point_max;
    Hit_Point hit_points[16];
    
    f32 distance_limit;
    Entity_Reference sword;

    // NOTE(xkazu0x): only for stairwells
    Vec2 walkable_dim;
    f32 walkable_height;
};

struct Sim_Entity_Hash {
    Sim_Entity *ptr;
    u32 index;
};

struct Sim_Region {
    // TODO(xkazu0x): need a hash table here to map stored entities indices
    // to sim entities
    
    World *world;
    f32 max_entity_radius;
    f32 max_entity_velocity;
    
    World_Position origin;
    Rect3 bounds;
    Rect3 updatable_bounds;
    
    u32 max_entity_count;
    u32 entity_count;
    Sim_Entity *entities;

    f32 base_ground_z;
    
    // TODO(xkazu0x): do i really want a hash for this??
    // NOTE(xkazu0x): must be a power of two!
    Sim_Entity_Hash hash[4096];
};

#endif // EXCALIBUR_SIMULATION_H
