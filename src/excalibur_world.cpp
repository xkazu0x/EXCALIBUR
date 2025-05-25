#define WORLD_CHUNK_SAFE_MARGIN (s32_max/64)
#define WORLD_CHUNK_UNINITIALIZED s32_max
#define TILES_PER_CHUNK 8

inline World_Position
null_position(void) {
    World_Position result = {};
    result.chunk_x = WORLD_CHUNK_UNINITIALIZED;
    return(result);
}

inline b32
is_valid(World_Position pos) {
    b32 result = (pos.chunk_x != WORLD_CHUNK_UNINITIALIZED);
    return(result);
}

inline b32
is_canonical(f32 chunk_dim, f32 tile_rel) {
    f32 epsilon = 0.1f;
    b32 result = ((tile_rel >= -(0.5f*chunk_dim + epsilon)) &&
                  (tile_rel <= (0.5f*chunk_dim + epsilon)));
    return(result);
}

inline b32
is_canonical(World *world, Vec3 offset) {
    b32 result = (is_canonical(world->chunk_dim_in_meters.x, offset.x) &&
                  is_canonical(world->chunk_dim_in_meters.y, offset.y) &&
                  is_canonical(world->chunk_dim_in_meters.z, offset.z));
    return(result);
}

internal b32
are_in_same_chunk(World *world, World_Position *a, World_Position *b) {
    assert(is_canonical(world, a->offset_));
    assert(is_canonical(world, b->offset_));
    
    b32 result = ((a->chunk_x == b->chunk_x) &&
                  (a->chunk_y == b->chunk_y) &&
                  (a->chunk_z == b->chunk_z));
    
    return(result);
}

inline World_Chunk *
get_world_chunk(World *world, s32 chunk_x, s32 chunk_y, s32 chunk_z,
                Arena *arena = 0) {
    assert(chunk_x > -WORLD_CHUNK_SAFE_MARGIN);
    assert(chunk_y > -WORLD_CHUNK_SAFE_MARGIN);
    assert(chunk_z > -WORLD_CHUNK_SAFE_MARGIN);
    assert(chunk_x < WORLD_CHUNK_SAFE_MARGIN);
    assert(chunk_y < WORLD_CHUNK_SAFE_MARGIN);
    assert(chunk_z < WORLD_CHUNK_SAFE_MARGIN);
    
    // TODO(xkazu0x): BETTER HASH FUNCTION!!!!
    u32 hash_value = 19*chunk_x + 7*chunk_y + 3*chunk_z;
    u32 hash_slot = hash_value & (array_count(world->chunk_hash) - 1);
    assert(hash_slot < array_count(world->chunk_hash));
    
    World_Chunk *chunk = world->chunk_hash + hash_slot;
    do {
        if ((chunk_x == chunk->chunk_x) &&
            (chunk_y == chunk->chunk_y) &&
            (chunk_z == chunk->chunk_z)) {
            break;
        }

        if (arena && (chunk->chunk_x != WORLD_CHUNK_UNINITIALIZED) && (!chunk->next_in_hash)) {
            chunk->next_in_hash = push_struct(arena, World_Chunk);
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
initialize_world(World *world, Vec3 chunk_dim_in_meters) {
    world->chunk_dim_in_meters = chunk_dim_in_meters;
    world->first_free = 0;
    for (u32 chunk_index = 0;
         chunk_index < array_count(world->chunk_hash);
         ++chunk_index) {
        world->chunk_hash[chunk_index].chunk_x = WORLD_CHUNK_UNINITIALIZED;
        world->chunk_hash[chunk_index].entity_block.entity_count = 0;
    }
}

inline void
recanonicalize_coord(f32 chunk_dim, s32 *tile, f32 *tile_rel) {
    // NOTE(xkazu0x): wrapping is not allowed, so all coordinates are assumed
    // to be within the safe margin
    // TODO(xkazu0x): assert that we are nowhere near the edges of the world
    
    s32 offset = round_f32_to_s32(*tile_rel/chunk_dim);
    *tile += offset;
    *tile_rel -= offset*chunk_dim;

    assert(is_canonical(chunk_dim, *tile_rel));
}

inline World_Position
map_into_chunk_space(World *world, World_Position base_pos, Vec3 offset) {
    World_Position result = base_pos;
    
    result.offset_ += offset;
    recanonicalize_coord(world->chunk_dim_in_meters.x, &result.chunk_x, &result.offset_.x);
    recanonicalize_coord(world->chunk_dim_in_meters.y, &result.chunk_y, &result.offset_.y);
    recanonicalize_coord(world->chunk_dim_in_meters.z, &result.chunk_z, &result.offset_.z);
    
    return(result);
}

inline Vec3
subtract(World *world, World_Position *a, World_Position *b) {
    Vec3 delta_chunk = make_vec3((f32)a->chunk_x - (f32)b->chunk_x,
                                 (f32)a->chunk_y - (f32)b->chunk_y,
                                 (f32)a->chunk_z - (f32)b->chunk_z);
    
    Vec3 result = hadamard(world->chunk_dim_in_meters, delta_chunk) + (a->offset_ - b->offset_);

    return(result);
}

inline World_Position
centered_chunk_point(u32 chunk_x, u32 chunk_y, u32 chunk_z) {
    World_Position result = {};
    result.chunk_x = chunk_x;
    result.chunk_y = chunk_y;
    result.chunk_z = chunk_z;
    return(result);
}

inline World_Position
centered_chunk_point(World_Chunk *chunk) {
    World_Position result = centered_chunk_point(chunk->chunk_x, chunk->chunk_y, chunk->chunk_z);
    return(result);
}

inline void
change_entity_location_raw(Arena *arena, World *world, u32 low_entity_index,
                           World_Position *old_pos, World_Position *new_pos) {
    assert(!old_pos || is_valid(*old_pos));
    assert(!new_pos || is_valid(*new_pos));
    
    if (old_pos && new_pos && are_in_same_chunk(world, old_pos, new_pos)) {
        // NOTE(xkazu0x): leave entity where it is
    } else {
        if (old_pos) {
            // NOTE(xkazu0x): pull the entity out of its old entity block
            World_Chunk *chunk = get_world_chunk(world, old_pos->chunk_x, old_pos->chunk_y, old_pos->chunk_z);
            assert(chunk);
            if (chunk) {
                b32 not_found = true;
                World_Entity_Block *first_block = &chunk->entity_block;
                for (World_Entity_Block *block = first_block;
                     block && not_found;
                     block = block->next) {
                    for (u32 index = 0;
                         (index < block->entity_count) && not_found;
                         index++) {
                        if (block->low_entity_index[index] == low_entity_index) {
                            assert(first_block->entity_count > 0);
                            block->low_entity_index[index] =
                                first_block->low_entity_index[--first_block->entity_count];
                            if (first_block->entity_count == 0) {
                                if (first_block->next) {
                                    World_Entity_Block *next_block = first_block->next;
                                    *first_block = *next_block;

                                    next_block->next = world->first_free;
                                    world->first_free = next_block;
                                }
                            }
                            
                            not_found = false;
                        }
                    }
                }
            }
        }
        if (new_pos) {
            // NOTE(xkazu0x): insert the entity into its new entity block
            World_Chunk *chunk = get_world_chunk(world, new_pos->chunk_x, new_pos->chunk_y, new_pos->chunk_z, arena);
            assert(chunk);
        
            World_Entity_Block *block = &chunk->entity_block;
            if (block->entity_count == array_count(block->low_entity_index)) {
                // NOTE(xkazu0x): we're out of room, get a new block
                World_Entity_Block *old_block = world->first_free;
                if (old_block) {
                    world->first_free = old_block->next;
                } else {
                    old_block = push_struct(arena, World_Entity_Block);
                }
            
                *old_block = *block;
                block->next = old_block;
                block->entity_count = 0;
            }

            assert(block->entity_count < array_count(block->low_entity_index));
            block->low_entity_index[block->entity_count++] = low_entity_index;
        }
    }
}

internal void
change_entity_location(Arena *arena, World *world,
                       u32 low_entity_index, Low_Entity *low_entity,
                       World_Position new_pos_init)
{
    World_Position *old_pos = 0;
    World_Position *new_pos = 0;

    if (!is_entity_flag_set(&low_entity->sim, EntityFlag_NonSpatial) &&
        (is_valid(low_entity->pos))) {
        old_pos = &low_entity->pos;
    }
    if (is_valid(new_pos_init)) {
        new_pos = &new_pos_init;
    }
    
    change_entity_location_raw(arena, world, low_entity_index, old_pos, new_pos);
    
    if (new_pos) {
        low_entity->pos = *new_pos;
        clear_entity_flags(&low_entity->sim, EntityFlag_NonSpatial);
    } else {
        low_entity->pos = null_position();
        add_entity_flags(&low_entity->sim, EntityFlag_NonSpatial);
    }
}
