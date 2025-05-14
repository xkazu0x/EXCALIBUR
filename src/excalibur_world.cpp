#define WORLD_CHUNK_SAFE_MARGIN (s32_max/64)
#define WORLD_CHUNK_UNINITIALIZED s32_max
#define TILES_PER_CHUNK 16

inline world_position_t
null_position(void) {
    world_position_t result = {};
    result.chunk_x = WORLD_CHUNK_UNINITIALIZED;
    return(result);
}

inline b32
is_valid(world_position_t pos) {
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
is_canonical(world_t *world, Vec3 offset) {
    b32 result = (is_canonical(world->chunk_dim_in_meters.x, offset.x) &&
                  is_canonical(world->chunk_dim_in_meters.y, offset.y) &&
                  is_canonical(world->chunk_dim_in_meters.z, offset.z));
    return(result);
}

internal b32
are_in_same_chunk(world_t *world, world_position_t *a, world_position_t *b) {
    Assert(is_canonical(world, a->offset_));
    Assert(is_canonical(world, b->offset_));
    
    b32 result = ((a->chunk_x == b->chunk_x) &&
                  (a->chunk_y == b->chunk_y) &&
                  (a->chunk_z == b->chunk_z));
    
    return(result);
}

inline world_chunk_t *
get_world_chunk(world_t *world, s32 chunk_x, s32 chunk_y, s32 chunk_z,
                Arena *arena = 0) {
    Assert(chunk_x > -WORLD_CHUNK_SAFE_MARGIN);
    Assert(chunk_y > -WORLD_CHUNK_SAFE_MARGIN);
    Assert(chunk_z > -WORLD_CHUNK_SAFE_MARGIN);
    Assert(chunk_x < WORLD_CHUNK_SAFE_MARGIN);
    Assert(chunk_y < WORLD_CHUNK_SAFE_MARGIN);
    Assert(chunk_z < WORLD_CHUNK_SAFE_MARGIN);
    
    // TODO(xkazu0x): BETTER HASH FUNCTION!!!!
    u32 hash_value = 19*chunk_x + 7*chunk_y + 3*chunk_z;
    u32 hash_slot = hash_value & (ArrayCount(world->chunk_hash) - 1);
    Assert(hash_slot < ArrayCount(world->chunk_hash));
    
    world_chunk_t *chunk = world->chunk_hash + hash_slot;
    do {
        if ((chunk_x == chunk->chunk_x) &&
            (chunk_y == chunk->chunk_y) &&
            (chunk_z == chunk->chunk_z)) {
            break;
        }

        if (arena && (chunk->chunk_x != WORLD_CHUNK_UNINITIALIZED) && (!chunk->next_in_hash)) {
            chunk->next_in_hash = push_struct(arena, world_chunk_t);
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
initialize_world(world_t *world, f32 tile_side_in_meters, f32 tile_depth_in_meters) {
    world->tile_side_in_meters = tile_side_in_meters;
    world->tile_depth_in_meters = tile_depth_in_meters;
    world->chunk_dim_in_meters = make_vec3(((f32)TILES_PER_CHUNK)*tile_side_in_meters,
                                           ((f32)TILES_PER_CHUNK)*tile_side_in_meters,
                                           tile_depth_in_meters);
    world->first_free = 0;
    for (u32 chunk_index = 0;
         chunk_index < ArrayCount(world->chunk_hash);
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

    Assert(is_canonical(chunk_dim, *tile_rel));
}

inline world_position_t
map_into_chunk_space(world_t *world, world_position_t base_pos, Vec3 offset) {
    world_position_t result = base_pos;
    
    result.offset_ += offset;
    recanonicalize_coord(world->chunk_dim_in_meters.x, &result.chunk_x, &result.offset_.x);
    recanonicalize_coord(world->chunk_dim_in_meters.y, &result.chunk_y, &result.offset_.y);
    recanonicalize_coord(world->chunk_dim_in_meters.z, &result.chunk_z, &result.offset_.z);
    
    return(result);
}

inline world_position_t
chunk_position_from_tile_position(world_t *world, s32 tile_x, s32 tile_y, s32 tile_z,
                                  Vec3 additional_offset = make_vec3(0.0f)) {
    world_position_t base_pos = {};

    Vec3 tile_dim = make_vec3(world->tile_side_in_meters, world->tile_side_in_meters, world->tile_depth_in_meters);
    Vec3 offset = hadamard_product(tile_dim, make_vec3((f32)tile_x, (f32)tile_y, (f32)tile_z));
    world_position_t result = map_into_chunk_space(world, base_pos, additional_offset + offset);
    
    Assert(is_canonical(world, result.offset_));
    
    return(result);
}

inline Vec3
subtract(world_t *world, world_position_t *a, world_position_t *b) {
    Vec3 delta_chunk = make_vec3((f32)a->chunk_x - (f32)b->chunk_x,
                                 (f32)a->chunk_y - (f32)b->chunk_y,
                                 (f32)a->chunk_z - (f32)b->chunk_z);
    
    Vec3 result = hadamard_product(world->chunk_dim_in_meters, delta_chunk) + (a->offset_ - b->offset_);

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
change_entity_location_raw(Arena *arena, world_t *world, u32 low_entity_index,
                           world_position_t *old_pos, world_position_t *new_pos) {
    Assert(!old_pos || is_valid(*old_pos));
    Assert(!new_pos || is_valid(*new_pos));
    
    if (old_pos && new_pos && are_in_same_chunk(world, old_pos, new_pos)) {
        // NOTE(xkazu0x): leave entity where it is
    } else {
        if (old_pos) {
            // NOTE(xkazu0x): pull the entity out of its old entity block
            world_chunk_t *chunk = get_world_chunk(world, old_pos->chunk_x, old_pos->chunk_y, old_pos->chunk_z);
            Assert(chunk);
            if (chunk) {
                b32 not_found = true;
                world_entity_block_t *first_block = &chunk->entity_block;
                for (world_entity_block_t *block = first_block;
                     block && not_found;
                     block = block->next) {
                    for (u32 index = 0;
                         (index < block->entity_count) && not_found;
                         index++) {
                        if (block->low_entity_index[index] == low_entity_index) {
                            Assert(first_block->entity_count > 0);
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
                            
                            not_found = false;
                        }
                    }
                }
            }
        }
        if (new_pos) {
            // NOTE(xkazu0x): insert the entity into its new entity block
            world_chunk_t *chunk = get_world_chunk(world, new_pos->chunk_x, new_pos->chunk_y, new_pos->chunk_z, arena);
            Assert(chunk);
        
            world_entity_block_t *block = &chunk->entity_block;
            if (block->entity_count == ArrayCount(block->low_entity_index)) {
                // NOTE(xkazu0x): we're out of room, get a new block
                world_entity_block_t *old_block = world->first_free;
                if (old_block) {
                    world->first_free = old_block->next;
                } else {
                    old_block = push_struct(arena, world_entity_block_t);
                }
            
                *old_block = *block;
                block->next = old_block;
                block->entity_count = 0;
            }

            Assert(block->entity_count < ArrayCount(block->low_entity_index));
            block->low_entity_index[block->entity_count++] = low_entity_index;
        }
    }
}

internal void
change_entity_location(Arena *arena, world_t *world,
                       u32 low_entity_index, low_entity_t *low_entity,
                       world_position_t new_pos_init)
{
    world_position_t *old_pos = 0;
    world_position_t *new_pos = 0;

    if (!is_entity_flag_set(&low_entity->sim, ENTITY_FLAG_NON_SPATIAL) &&
        (is_valid(low_entity->pos))) {
        old_pos = &low_entity->pos;
    }
    if (is_valid(new_pos_init)) {
        new_pos = &new_pos_init;
    }
    
    change_entity_location_raw(arena, world, low_entity_index, old_pos, new_pos);
    
    if (new_pos) {
        low_entity->pos = *new_pos;
        clear_entity_flags(&low_entity->sim, ENTITY_FLAG_NON_SPATIAL);
    } else {
        low_entity->pos = null_position();
        add_entity_flags(&low_entity->sim, ENTITY_FLAG_NON_SPATIAL);
    }
}
