#define WORLD_CHUNK_SAFE_MARGIN (s32_max/64)
#define WORLD_CHUNK_UNINITIALIZED s32_max

inline world_chunk_t *
get_world_chunk(world_t *world, s32 chunk_x, s32 chunk_y, s32 chunk_z,
                memory_arena_t *arena = 0) {
    EX_ASSERT(chunk_x > -WORLD_CHUNK_SAFE_MARGIN);
    EX_ASSERT(chunk_y > -WORLD_CHUNK_SAFE_MARGIN);
    EX_ASSERT(chunk_z > -WORLD_CHUNK_SAFE_MARGIN);
    EX_ASSERT(chunk_x < WORLD_CHUNK_SAFE_MARGIN);
    EX_ASSERT(chunk_y < WORLD_CHUNK_SAFE_MARGIN);
    EX_ASSERT(chunk_z < WORLD_CHUNK_SAFE_MARGIN);
    
    // TODO(xkazu0x): BETTER HASH FUNCTION!!!!
    u32 hash_value = 19*chunk_x + 7*chunk_y + 3*chunk_z;
    u32 hash_slot = hash_value & (EX_ARRAY_COUNT(world->chunk_hash) - 1);
    EX_ASSERT(hash_slot < EX_ARRAY_COUNT(world->chunk_hash));
    
    world_chunk_t *chunk = world->chunk_hash + hash_slot;
    do {
        if ((chunk_x == chunk->chunk_x) &&
            (chunk_y == chunk->chunk_y) &&
            (chunk_z == chunk->chunk_z)) {
            break;
        }

        if (arena && (chunk->chunk_x != WORLD_CHUNK_UNINITIALIZED) && (!chunk->next_in_hash)) {
            chunk->next_in_hash = (world_chunk_t *)memory_arena_push(arena, sizeof(world_chunk_t));
            chunk = chunk->next_in_hash;
            chunk->chunk_x = WORLD_CHUNK_UNINITIALIZED;
        }

        if (arena && (chunk->chunk_x == WORLD_CHUNK_UNINITIALIZED)) {
            u32 tile_count = world->chunk_dim*world->chunk_dim;

            chunk->chunk_x = chunk_x;
            chunk->chunk_y = chunk_y;
            chunk->chunk_z = chunk_z;
            
            chunk->next_in_hash = 0;
            break;
        }
        
        chunk = chunk->next_in_hash;
    } while(chunk);
    return(chunk);
}

#if 0
inline world_chunk_position_t
get_world_position(world_t *world, s32 tile_x, s32 tile_y, s32 tile_z) {
    world_position_t result;
    result.tile_chunk_x = tile_x >> world->chunk_shift;
    result.tile_chunk_y = tile_y >> world->chunk_shift;
    result.tile_chunk_z = tile_z;
    result.tile_x = tile_x & world->chunk_mask;
    result.tile_y = tile_y & world->chunk_mask;
    return(result);
}
#endif

internal void
world_initialize(world_t *world, f32 tile_size_in_meters) {
    world->tile_size_in_meters = tile_size_in_meters;
    
    world->chunk_shift = 4;
    world->chunk_mask = (1 << world->chunk_shift) - 1;
    world->chunk_dim = (1 << world->chunk_shift);
    for (u32 chunk_index = 0;
         chunk_index < EX_ARRAY_COUNT(world->chunk_hash);
         chunk_index++) {
        world->chunk_hash[chunk_index].chunk_x = WORLD_CHUNK_UNINITIALIZED;
    }
}

/////////////////
// TODO(xkazu0x):

inline void
recanonicalize_coord(world_t *world, s32 *tile_index, f32 *tile_rel) {
    s32 offset = round_f32_to_s32(*tile_rel / world->tile_size_in_meters);
    *tile_index += offset;
    *tile_rel -= offset*world->tile_size_in_meters;

    EX_ASSERT(*tile_rel > -0.5f*world->tile_size_in_meters);
    EX_ASSERT(*tile_rel < 0.5f*world->tile_size_in_meters);
}

inline world_position_t
map_into_tile_space(world_t *world, world_position_t base_pos, vec2f offset) {
    world_position_t result = base_pos;
    result.tile_offset_ += offset;
    recanonicalize_coord(world, &result.tile_x, &result.tile_offset_.x);
    recanonicalize_coord(world, &result.tile_y, &result.tile_offset_.y);
    return(result);
}

internal b32
are_on_the_same_tile(world_position_t *a, world_position_t *b) {
    b32 result = ((a->tile_x == b->tile_x) &&
                  (a->tile_y == b->tile_y) &&
                  (a->tile_z == b->tile_z));
    return(result);
}

inline vec3f
subtract(world_t *world, world_position_t *a, world_position_t *b) {
    vec3f result = {};

    f32 tile_dx = (f32)a->tile_x - (f32)b->tile_x;
    f32 tile_dy = (f32)a->tile_y - (f32)b->tile_y;
    f32 tile_dz = (f32)a->tile_z - (f32)b->tile_z;
    
    result.x = world->tile_size_in_meters*tile_dx + (a->tile_offset_.x - b->tile_offset_.x);
    result.y = world->tile_size_in_meters*tile_dy + (a->tile_offset_.y - b->tile_offset_.y);
    result.z = world->tile_size_in_meters*tile_dz;

    return(result);
}

inline world_position_t
centered_tile_point(u32 tile_x, u32 tile_y, u32 tile_z) {
    world_position_t result = {};
    result.tile_x = tile_x;
    result.tile_y = tile_y;
    result.tile_z = tile_z;
    return(result);
}
