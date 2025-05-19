#ifndef EXCALIBUR_GAME_H
#define EXCALIBUR_GAME_H

/*
  TODO(xkazu0x):
  ARCHITECTURE EXPLORATION
  - Debug code
    - logging
    - diagramming
    - a little GUI
    
  - Asset streaming
  
  - Audio
    - sound effect triggers
    - ambient sounds
    - music

  - AI
    - Rudimentary monster behavior example
    - Pathfinding
    - AI "storage"
  
  - Animation
    - Skeletal animation
    - Particle system

  RENDERING
  - draw centered bitmap??
*/

// NOTE(xkazu0x):
// EXCALIBUR_INTERNAL:
// > 0 - build for public release
// > 1 - build for develop only
// EXCALIBUR_DEBUG:
// > 0 - not slow code allowed
// > 1 - slow code welcome

struct Arena {
    memi reserve_size;
    memi used;
    u8 *base_memory;

    s32 temp_count;
};

internal Arena
make_arena(memi reserve_size, void *base_memory) {
    Arena result;
    result.reserve_size = reserve_size;
    result.used = 0;
    result.base_memory = (u8 *)base_memory;
    result.temp_count = 0;
    return(result);
}

internal void *
arena_push_(Arena *arena, memi size) {
    Assert((arena->used + size) <= arena->reserve_size);
    void *result = arena->base_memory + arena->used;
    arena->used += size;
    return(result);
}

#define push_size(arena, size) arena_push_(arena, size)
#define push_struct(arena, type) (type *)arena_push_(arena, sizeof(type))
#define push_array(arena, type, count) (type *)arena_push_(arena, sizeof(type)*(count))

struct Temporary_Memory {
    Arena *arena;
    memi used;
};

internal Temporary_Memory
begin_temporary_memory(Arena *arena) {
    Temporary_Memory result;
    result.arena = arena;
    result.used = arena->used;
    ++arena->temp_count;
    return(result);
}

internal void
end_temporary_memory(Temporary_Memory *temp_mem) {
    Arena *arena = temp_mem->arena;
    
    Assert(arena->used >= temp_mem->used);
    arena->used = temp_mem->used;

    Assert(arena->temp_count > 0);
    --arena->temp_count;
}

internal void
check_arena(Arena *arena) {
    Assert(arena->temp_count == 0);
}

internal inline void
zero_size(memi size, void *ptr) {
    u8 *byte = (u8 *)ptr;
    while (size--) {
        *byte++ = 0;
    }
}
#define zero_struct(instance) zero_size(sizeof(instance), &(instance))

#define BITMAP_BYTES_PER_PIXEL 4
struct Bitmap {
    s32 width;
    s32 height;
    s32 pitch;
    void *memory;
};

struct Low_Entity {
    // TODO(xkazu0x): it's kind of busted that pos's can be invalid here,
    // AND we store whether they would be invalid in the flags field...
    // can we do something better here?
    World_Position pos;
    Sim_Entity sim;
};

struct Controlled_Player {
    u32 entity_index;
    // NOTE(xkazu0x): these are the gamepad requests for simulation
    Vec2 dd_pos;
    Vec2 d_sword;
    f32 d_z;
};

struct Pairwise_Collision_Rule {
    b32 can_collide;
    u32 storage_index_a;
    u32 storage_index_b;

    Pairwise_Collision_Rule *next_in_hash;
};

struct Game_State;
internal void add_collision_rule(Game_State *game_state, u32 storage_index_a, u32 storage_index_b, b32 should_collide);
internal void clear_collision_rules_for(Game_State *game_state, u32 storage_index);

struct Ground_Buffer {
    // NOTE(xkazu0x): An invalid position tells us that
    // this Ground_Buffer has not been filled
    World_Position position; // NOTE(xkazu0x): center of the bitmap
    Bitmap bitmap;
};

struct Game_State {
    Arena world_arena;
    World *world;

    f32 typical_floor_height;
    
    u32 camera_following_entity_index;
    World_Position camera_pos;
    
    Controlled_Player controlled_players[Gamepad_Count];
    
    // TODO(xkazu0x): change the name to "stored entity"
    u32 low_entity_count;
    Low_Entity low_entities[100000];

    f32 meters_to_pixels;
    f32 pixels_to_meters;

    Bitmap wall_sprite;
    Bitmap stairwell_sprite;
    Bitmap grass_sprites[2];
    Bitmap stone_sprites[2];
    Bitmap tuft_sprites[2];
    Bitmap shadow_sprite;
    Bitmap player_sprites[4];
    Bitmap bat_sprite;
    Bitmap sword_sprite;
    
    // TODO(xkazu0x): must be power of two
    Pairwise_Collision_Rule *collision_rule_hash[256];
    Pairwise_Collision_Rule *first_free_collision_rule;

    Sim_Entity_Collision_Volume_Group *null_collision;
    Sim_Entity_Collision_Volume_Group *standard_room_collision;
    Sim_Entity_Collision_Volume_Group *wall_collision;
    Sim_Entity_Collision_Volume_Group *stair_collision;
    Sim_Entity_Collision_Volume_Group *sword_collision;
    Sim_Entity_Collision_Volume_Group *player_collision;
    Sim_Entity_Collision_Volume_Group *monster_collision;
    Sim_Entity_Collision_Volume_Group *familiar_collision;

    f32 time;
};

struct Transient_State {
    b32 initialized;
    Arena arena;

    u32 ground_buffer_count;
    Ground_Buffer *ground_buffers;
};

internal Low_Entity *
get_low_entity(Game_State *game_state, u32 index) {
    Low_Entity *result = 0;
    if ((index > 0) && (index < game_state->low_entity_count)) {
        result = game_state->low_entities + index;
    }
    return(result);
}

#endif // EXCALIBUR_GAME_H
