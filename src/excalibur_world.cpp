#define WORLD_CHUNK_SAFE_MARGIN (s32_max/64)
#define WORLD_CHUNK_UNINITIALIZED s32_max
#define TILES_PER_CHUNK 16

inline b32
is_canonical(world_t *world, f32 tile_rel) {
    b32 result = ((tile_rel >= -0.5f*world->chunk_side_in_meters) &&
                  (tile_rel <= 0.5f*world->chunk_side_in_meters));
    return(result);
}

inline b32
is_canonical(world_t *world, vec2f offset) {
    b32 result = (is_canonical(world, offset.x) && is_canonical(world, offset.y));
    return(result);
}

internal b32
are_in_same_chunk(world_t *world, world_position_t *a, world_position_t *b) {
    EX_ASSERT(is_canonical(world, a->offset_));
    EX_ASSERT(is_canonical(world, b->offset_));
    
    b32 result = ((a->chunk_x == b->chunk_x) &&
                  (a->chunk_y == b->chunk_y) &&
                  (a->chunk_z == b->chunk_z));
    return(result);
}

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

internal void
world_initialize(world_t *world, f32 tile_side_in_meters) {
    world->tile_side_in_meters = tile_side_in_meters;
    world->chunk_side_in_meters = (f32)TILES_PER_CHUNK*tile_side_in_meters;
    world->first_free = 0;
        
    for (u32 chunk_index = 0;
         chunk_index < EX_ARRAY_COUNT(world->chunk_hash);
         chunk_index++) {
        world->chunk_hash[chunk_index].chunk_x = WORLD_CHUNK_UNINITIALIZED;
        world->chunk_hash[chunk_index].entity_block.entity_count = 0;
    }
}

inline void
recanonicalize_coord(world_t *world, s32 *chunk_index, f32 *chunk_offset) {
    // NOTE(xkazu0x): wrapping is not allowed, so all coordinates are assumed
    // to be within the safe margin
    // TODO(xkazu0x): assert that we are nowhere near the edges of the world
    
    s32 offset = round_f32_to_s32(*chunk_offset / world->chunk_side_in_meters);
    *chunk_index += offset;
    *chunk_offset -= offset*world->chunk_side_in_meters;

    EX_ASSERT(is_canonical(world, *chunk_offset));
}

inline world_position_t
map_into_chunk_space(world_t *world, world_position_t base_pos, vec2f offset) {
    world_position_t result = base_pos;
    result.offset_ += offset;
    recanonicalize_coord(world, &result.chunk_x, &result.offset_.x);
    recanonicalize_coord(world, &result.chunk_y, &result.offset_.y);
    return(result);
}

inline world_position_t
chunk_position_from_tile_position(world_t *world, s32 tile_x, s32 tile_y, s32 tile_z) {
    world_position_t result = {};
    
    result.chunk_x = tile_x / TILES_PER_CHUNK;
    result.chunk_y = tile_y / TILES_PER_CHUNK;
    result.chunk_z = tile_z / TILES_PER_CHUNK;
    
    result.offset_.x = (f32)(tile_x - (result.chunk_x*TILES_PER_CHUNK))*world->tile_side_in_meters;
    result.offset_.y = (f32)(tile_y - (result.chunk_y*TILES_PER_CHUNK))*world->tile_side_in_meters;
    // TODO(xkazu0x): move to 3D z!!
    
    return(result);
}

inline vec3f
subtract(world_t *world, world_position_t *a, world_position_t *b) {
    vec3f result = {};

    f32 chunk_dx = (f32)a->chunk_x - (f32)b->chunk_x;
    f32 chunk_dy = (f32)a->chunk_y - (f32)b->chunk_y;
    f32 chunk_dz = (f32)a->chunk_z - (f32)b->chunk_z;
    
    result.x = world->chunk_side_in_meters*chunk_dx + (a->offset_.x - b->offset_.x);
    result.y = world->chunk_side_in_meters*chunk_dy + (a->offset_.y - b->offset_.y);
    result.z = world->chunk_side_in_meters*chunk_dz;

    return(result);
}

inline world_position_t
centered_chunk_point(u32 chunk_x, u32 chunk_y, u32 chunk_z) {
    world_position_t result = {};
    result.chunk_x = chunk_x;
    result.chunk_y = chunk_y;
    result.chunk_z = chunk_z;
    return(result);
}

inline void
entity_change_location(memory_arena_t *arena, world_t *world, u32 low_entity_index,
                       world_position_t *old_pos, world_position_t *new_pos) {
    if (old_pos && are_in_same_chunk(world, old_pos, new_pos)) {
        // NOTE(xkazu0x): leave entity where it is
    } else {
        if (old_pos) {
            // NOTE(xkazu0x): pull the entity out of its old entity block
            world_chunk_t *chunk = get_world_chunk(world, old_pos->chunk_x, old_pos->chunk_y, old_pos->chunk_z);
            EX_ASSERT(chunk);
            if (chunk) {
                b32 not_found = EX_TRUE;
                world_entity_block_t *first_block = &chunk->entity_block;
                for (world_entity_block_t *block = first_block;
                     block && not_found;
                     block = block->next) {
                    for (u32 index = 0;
                         (index < block->entity_count) && not_found;
                         index++) {
                        if (block->low_entity_index[index] == low_entity_index) {
                            EX_ASSERT(first_block->entity_count > 0);
                            block->low_entity_index[index] =
                                first_block->low_entity_index[--first_block->entity_count];
                            if (first_block->entity_count == 0) {
                                if (first_block->next) {
                                    world_entity_block_t *next_block = first_block->next;
                                    *first_block = *next_block;

                                    next_block->next = world->first_free;
                                    world->first_free = next_block;
                                }
                            }
                            
                            not_found = EX_FALSE;
                        }
                    }
                }
            }
        }
        
        // NOTE(xkazu0x): insert the entity into its new entity block
        world_chunk_t *chunk = get_world_chunk(world, new_pos->chunk_x, new_pos->chunk_y, new_pos->chunk_z, arena);
        EX_ASSERT(chunk);
        
        world_entity_block_t *block = &chunk->entity_block;
        if (block->entity_count == EX_ARRAY_COUNT(block->low_entity_index)) {
            // NOTE(xkazu0x): we're out of room, get a new block
            world_entity_block_t *old_block = world->first_free;
            if (old_block) {
                world->first_free = old_block->next;
            } else {
                old_block = (world_entity_block_t *)memory_arena_push(arena, sizeof(world_entity_block_t));
            }
            
            *old_block = *block;
            block->next = old_block;
            block->entity_count = 0;
        }

        EX_ASSERT(block->entity_count < EX_ARRAY_COUNT(block->low_entity_index));
        block->low_entity_index[block->entity_count++] = low_entity_index;
    }
}
