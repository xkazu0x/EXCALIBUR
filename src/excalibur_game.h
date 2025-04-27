#ifndef EXCALIBUR_GAME_H
#define EXCALIBUR_GAME_H

// NOTE(xkazu0x):
// EXCALIBUR_INTERNAL:
// > 0 - build for public release
// > 1 - build for develop only
// EXCALIBUR_DEBUG:
// > 0 - not slow code allowed
// > 1 - slow code welcome

struct memory_arena_t {
    memi size;
    memi used;
    u8 *base;
};

internal memory_arena_t
memory_arena_create(memi size, void *base) {
    memory_arena_t result;
    result.size = size;
    result.used = 0;
    result.base = (u8 *)base;
    return(result);
}

internal void *
memory_arena_push(memory_arena_t *arena, memi size) {
    EX_ASSERT((arena->used + size) <= arena->size);
    void *result = arena->base + arena->used;
    arena->used += size;
    return(result);
}

#define zero_struct(instance) zero_size(sizeof(instance), &(instance))

internal inline void
zero_size(memi size, void *ptr) {
    u8 *byte = (u8 *)ptr;
    while (size--) {
        *byte++ = 0;
    }
}

struct bitmap_t {
    s32 width;
    s32 height;
    u32 *pixels;
};

struct low_entity_t {
    // TODO(xkazu0x): it's kind of busted that pos's can be invalid here,
    // AND we store whether they would be invalid in the flags field...
    // can we do something better here?
    world_position_t pos;
    sim_entity_t sim;
};

struct entity_visible_piece_t {
    bitmap_t *bitmap;
    vec3f offset;
    
    vec4f color;
    vec2f size;
    
    f32 entity_zc;
};

struct controlled_player_t {
    u32 entity_index;
    // NOTE(xkazu0x): these are the gamepad requests for simulation
    vec2f dd_pos;
    vec2f d_sword;
    f32 d_z;
};

struct game_state_t {
    memory_arena_t world_arena;
    world_t *world;
    
    u32 camera_following_entity_index;
    world_position_t camera_pos;
    
    controlled_player_t controlled_players[GAMEPAD_COUNT_MAX];
    
    // TODO(xkazu0x): change the name to "stored entity"
    u32 low_entity_count;
    low_entity_t low_entities[100000];

    f32 meters_to_pixels;
    bitmap_t player_sprites[4];
    bitmap_t wall_sprite;
    bitmap_t bat_sprite;
    bitmap_t shadow_sprite;
    bitmap_t sword_sprite;
};

// TODO(xkazu0x): this is dumb, this should just be part of
// the renderer pushbuffer - add correction of coordinates
// in there and be done with it.
struct entity_visible_piece_group_t {
    game_state_t *game_state;
    u32 piece_count;
    entity_visible_piece_t pieces[32];
};

#endif // EXCALIBUR_GAME_H
