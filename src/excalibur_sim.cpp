internal sim_entity_t *
sim_add_entity(sim_region_t *region) {
    sim_entity_t *entity = 0;
    
    if (region->entity_count < region->max_entity_count) {
        entity = region->entities + region->entity_count++;

        // TODO(xkazu0x): see what we want to do about clearing policy when
        // the entity system is more fleshed out.
        *entity = {};
    } else {
        INVALID_CODE_PATH();
    }

    return(entity);
}

inline vec2f
get_sim_space_pos(sim_region_t *region, low_entity_t *stored) {
    // NOTE(xkazu0x): map the entity in camera space
    vec3f diff = subtract(region->world, &stored->pos, &region->origin);
    vec2f result = _vec2f(diff.x, diff.y);
    return(result);
}

internal sim_entity_t *
sim_add_entity(sim_region_t *region, low_entity_t *src, vec2f *sim_pos) {
    sim_entity_t *dest = sim_add_entity(region);
    
    if (dest) {
        // TODO(xkazu0x): convert the stored entity to a simulation entity
        if (sim_pos) {
            dest->pos = *sim_pos;
        } else {
            dest->pos = get_sim_space_pos(region, src);
        }
    }
    
    return(dest);
}

internal sim_region_t *
sim_begin(memory_arena_t *sim_arena, game_state_t *state, world_t *world, world_position_t origin, rect2f bounds) {
    // TODO(xkazu0x): if entities were stored in the world, we wouldn't need the game state here
    
    sim_region_t *region = (sim_region_t *)memory_arena_push(sim_arena, sizeof(sim_region_t));

    region->world = world;
    region->origin = origin;
    region->bounds = bounds;

    // TODO(xkazu0x): need to be more specific about entity counts
    region->max_entity_count = 4096;
    region->entity_count = 0;
    region->entities = (sim_entity_t *)memory_arena_push(sim_arena, region->max_entity_count*sizeof(sim_entity_t));
    
    world_position_t min_chunk_pos = map_into_chunk_space(world, region->origin, rect_get_min_corner(region->bounds));
    world_position_t max_chunk_pos = map_into_chunk_space(world, region->origin, rect_get_max_corner(region->bounds));
    
    for (s32 chunk_y = min_chunk_pos.chunk_y;
         chunk_y <= max_chunk_pos.chunk_y;
         ++chunk_y) {
        for (s32 chunk_x = min_chunk_pos.chunk_x;
             chunk_x <= max_chunk_pos.chunk_x;
             ++chunk_x) {
            world_chunk_t *chunk = get_world_chunk(world, chunk_x, chunk_y, region->origin.chunk_z);
            if (chunk) {
                for (world_entity_block_t *block = &chunk->entity_block;
                     block;
                     block = block->next) {
                    for (u32 entity_index_index = 0;
                         entity_index_index < block->entity_count;
                         entity_index_index++) {
                        u32 low_entity_index = block->low_entity_index[entity_index_index];
                        low_entity_t *low = state->low_entities + low_entity_index;
                        vec2f sim_space_pos = get_sim_space_pos(region, low);
                        if (is_in_rect(region->bounds, sim_space_pos)) {
                            sim_add_entity(region, low, &sim_space_pos);
                        }
                    }
                }
            }
        }
    }

    return(region);
}

internal void
sim_end(sim_region_t *region, game_state_t *state) {
    // TODO(xkazu0x): maybe don't take a game state here, low entities
    // should be stored in the world??
    
    sim_entity_t *entity = region->entities;
    for (u32 entity_index = 0;
         entity_index < region->entity_count;
         ++entity_index, ++entity) {
        low_entity_t *stored = state->low_entities + entity->storage_index;

        // TODO(xkazu0x): save stat back to the stored entity, once sim entities
        // do state decompression.

        world_position_t new_pos = map_into_chunk_space(region->world, region->origin, entity->pos);
        change_entity_location(&state->world_arena, state->world, entity->storage_index,
                               stored, &stored->pos, &new_pos);

        // TODO(xkazu0x): entity mapping hash table
        sim_entity_t camera_following_entity = force_entity_into_high(state, state->camera_following_entity_index);
        if (camera_following_entity.high) {
            world_position_t new_camera_pos = state->camera_pos;
            new_camera_pos.chunk_z = camera_following_entity.low->pos.chunk_z;
        
#if 0
            if (camera_following_entity.high->pos.x > (9.0f*world->tile_side_in_meters)) {
                new_camera_pos.tile_x += 17;
            }
            if (camera_following_entity.high->pos.x < -(9.0f*world->tile_side_in_meters)) {
                new_camera_pos.tile_x -= 17;
            }
            if (camera_following_entity.high->pos.y > (5.0f*world->tile_side_in_meters)) {
                new_camera_pos.tile_y += 9;
            }
            if (camera_following_entity.high->pos.y < -(5.0f*world->tile_side_in_meters)) {
                new_camera_pos.tile_y -= 9;
            }
#else
            new_camera_pos = camera_following_entity.low->pos;
#endif
        
            set_camera(state, new_camera_pos);
        }
    }
}
