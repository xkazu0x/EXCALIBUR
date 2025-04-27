internal sim_entity_hash_t *
get_hash_from_storage_index(sim_region_t *region, u32 storage_index) {
    EX_ASSERT(storage_index);
    sim_entity_hash_t *result = 0;
    
    u32 hash_value = storage_index;
    for (u32 offset = 0;
         offset < EX_ARRAY_COUNT(region->hash);
         ++offset) {
        u32 hash_mask = (EX_ARRAY_COUNT(region->hash) - 1);
        u32 hash_index = ((hash_value + offset) & (hash_mask));
        sim_entity_hash_t *entry = region->hash + hash_index;
        if (entry->index == 0 || entry->index == storage_index) {
            result = entry;
            break;
        }
    }
    
    return(result);
}

inline sim_entity_t *
get_entity_by_storage_index(sim_region_t *region, u32 storage_index) {
    sim_entity_hash_t *entry = get_hash_from_storage_index(region, storage_index);
    sim_entity_t *result = entry->ptr;
    return(result);
}

internal sim_entity_t *
add_sim_entity(game_state_t *state, sim_region_t *region, u32 storage_index, low_entity_t *stored, vec2f *pos);

inline vec2f
get_sim_space_pos(sim_region_t *region, low_entity_t *stored) {
    // NOTE(xkazu0x): map the entity in camera space
    vec2f result = INVALID_POS;
    
    if (!is_entity_flag_set(&stored->sim, ENTITY_FLAG_NON_SPATIAL)) {
        vec3f diff = subtract(region->world, &stored->pos, &region->origin);
        result = _vec2f(diff.x, diff.y);
    }
    
    return(result);
}

inline void
load_entity_reference(game_state_t *state, sim_region_t *region, entity_reference_t *reference) {
    if (reference->index) {
        sim_entity_hash_t *entry = get_hash_from_storage_index(region, reference->index);
        if (entry->ptr == 0) {
            entry->index = reference->index;
            
            low_entity_t *low_entity = get_low_entity(state, reference->index);
            vec2f pos = get_sim_space_pos(region, low_entity);
            
            entry->ptr = add_sim_entity(state, region, reference->index, low_entity, &pos);
        }
        reference->ptr = entry->ptr;
    }
}

inline void
store_entity_reference(entity_reference_t *reference) {
    if (reference->ptr != 0) {
        reference->index = reference->ptr->storage_index;
    }
}

internal sim_entity_t *
add_sim_entity_raw(game_state_t *state, sim_region_t *region,
                   u32 storage_index, low_entity_t *stored) {
    EX_ASSERT(storage_index);
    sim_entity_t *entity = 0;
    
    sim_entity_hash_t *entry = get_hash_from_storage_index(region, storage_index);
    if (entry->ptr == 0) {
        if (region->entity_count < region->max_entity_count) {
            entity = region->entities + region->entity_count++;

            entry->index = storage_index;
            entry->ptr = entity;
        
            if (stored) {
                // TODO(xkazu0x): this should really be a decompression step, not
                // a copy!
                *entity = stored->sim;
                load_entity_reference(state, region, &entity->sword);

                EX_ASSERT(!is_entity_flag_set(&stored->sim, ENTITY_FLAG_SIMMING));
                add_entity_flag(&stored->sim, ENTITY_FLAG_SIMMING);
            }
        
            entity->storage_index = storage_index;
            entity->updatable = false;
        } else {
            INVALID_CODE_PATH();
        }
    }

    return(entity);
}

internal sim_entity_t *
add_sim_entity(game_state_t *state, sim_region_t *region, u32 storage_index, low_entity_t *stored, vec2f *pos) {
    sim_entity_t *entity = add_sim_entity_raw(state, region, storage_index, stored);
    
    if (entity) {
        if (pos) {
            entity->pos = *pos;
            entity->updatable = is_in_rect(region->updatable_bounds, entity->pos);
        } else {
            entity->pos = get_sim_space_pos(region, stored);
        }
    }
    
    return(entity);
}

internal sim_region_t *
begin_sim(memory_arena_t *sim_arena, game_state_t *state, world_t *world, world_position_t origin, rect2f bounds) {
    // TODO(xkazu0x): if entities were stored in the world, we wouldn't need the game state here
    
    sim_region_t *region = (sim_region_t *)memory_arena_push(sim_arena, sizeof(sim_region_t));
    zero_struct(region->hash);

    // TODO(xkazu0x): IMPORTANT(xkazu0x): calculate this eventually from the maximun value
    // of all entities radius plus their speed
    f32 update_safety_margin = 1.0f;
    
    region->world = world;
    region->origin = origin;
    region->updatable_bounds = bounds;
    region->bounds = add_radius_to(region->updatable_bounds, update_safety_margin, update_safety_margin);
    
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
                    for (u32 entity_index = 0;
                         entity_index < block->entity_count;
                         entity_index++) {
                        u32 low_entity_index = block->low_entity_index[entity_index];
                        low_entity_t *low = state->low_entities + low_entity_index;
                        if (!is_entity_flag_set(&low->sim, ENTITY_FLAG_NON_SPATIAL))
                        {
                            vec2f sim_space_pos = get_sim_space_pos(region, low);
                            if (is_in_rect(region->bounds, sim_space_pos)) {
                                add_sim_entity(state, region, low_entity_index, low, &sim_space_pos);
                            }
                        }
                    }
                }
            }
        }
    }

    return(region);
}

internal void
end_sim(sim_region_t *region, game_state_t *state) {
    // TODO(xkazu0x): maybe don't take a game state here, low entities
    // should be stored in the world??
    
    sim_entity_t *entity = region->entities;
    for (u32 entity_index = 0;
         entity_index < region->entity_count;
         ++entity_index, ++entity)
    {
        low_entity_t *stored = state->low_entities + entity->storage_index;

        EX_ASSERT(is_entity_flag_set(&stored->sim, ENTITY_FLAG_SIMMING));
        stored->sim = *entity;
        EX_ASSERT(!is_entity_flag_set(&stored->sim, ENTITY_FLAG_SIMMING));
        
        store_entity_reference(&stored->sim.sword);
        
        // TODO(xkazu0x): save stat back to the stored entity, once sim entities
        // do state decompression.
        world_position_t new_pos = is_entity_flag_set(entity, ENTITY_FLAG_NON_SPATIAL) ?
            null_position() :
            map_into_chunk_space(region->world, region->origin, entity->pos);
        change_entity_location(&state->world_arena, state->world, entity->storage_index,
                               stored, new_pos);

        if (entity->storage_index == state->camera_following_entity_index) {
            world_position_t new_camera_pos = state->camera_pos;
            new_camera_pos.chunk_z = stored->pos.chunk_z;
        
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
            new_camera_pos = stored->pos;
#endif
            state->camera_pos = new_camera_pos;
        }
    }
}

internal b32
test_wall(f32 wall_x, f32 rel_x, f32 rel_y, f32 player_delta_x, f32 player_delta_y,
          f32 *t_min, f32 min_y, f32 max_y) {
    b32 hit = EX_FALSE;
    
    f32 t_epsilon = 0.01f;
    if (player_delta_x != 0.0f) {
        f32 t_result = (wall_x - rel_x) / player_delta_x;
        f32 y = rel_y + t_result*player_delta_y;
        if ((t_result >= 0.0f) && (*t_min > t_result)) {
            if (y >= min_y && (y <= max_y)) {
                *t_min = EX_MAX(0.0f, t_result - t_epsilon);
                hit = EX_TRUE;
            }
        }
    }
    
    return(hit);
}

internal void
move_entity(sim_region_t *region, sim_entity_t *entity, f32 delta, move_spec_t *move_spec, vec2f dd_pos) {
    EX_ASSERT(!is_entity_flag_set(entity, ENTITY_FLAG_NON_SPATIAL));
    
    world_t *world = region->world;

    if (move_spec->unit_max_accel_vector) {
        f32 dd_pos_length = vec_length_sqr(dd_pos);
        if (dd_pos_length > 1.0f) {
            dd_pos *= (1.0f/sqrt_f32(dd_pos_length));
        }
    }
    
    dd_pos *= move_spec->speed;
    
    // TODO(xkazu0x): ODE here!
    dd_pos += -move_spec->drag*entity->d_pos;
    
    //vec2f old_player_pos = entity->pos;
    vec2f player_delta = (0.5f*dd_pos*sqr(delta) + entity->d_pos*delta);
    entity->d_pos = dd_pos*delta + entity->d_pos;
    //vec2f new_player_pos = old_player_pos + player_delta;

    f32 dd_z = -9.8f;
    entity->z = 0.5f*dd_z*sqr(delta) + entity->d_z*delta + entity->z;
    entity->d_z = dd_z*delta + entity->d_z;
    if (entity->z < 0.0f) entity->z = 0.0f;
    
    for (u32 iteration = 0;
         iteration < 4;
         ++iteration) {
        f32 t_min = 1.0f;
        vec2f wall_normal = {};
        sim_entity_t *hit_entity = 0;

        vec2f desired_pos = entity->pos + player_delta;

        if (is_entity_flag_set(entity, ENTITY_FLAG_COLLIDES) &&
            !is_entity_flag_set(entity, ENTITY_FLAG_NON_SPATIAL))
        {
            // TODO(xkazu0x): spatial partition here!
            for (u32 test_high_entity_index = 0;
                 test_high_entity_index < region->entity_count;
                 ++test_high_entity_index)
            {
                sim_entity_t *test_entity = region->entities + test_high_entity_index;
                if (entity != test_entity)
                {
                    if (is_entity_flag_set(test_entity, ENTITY_FLAG_COLLIDES) &&
                        !is_entity_flag_set(entity, ENTITY_FLAG_NON_SPATIAL))
                    {
                        f32 dim_w = test_entity->width + entity->width;
                        f32 dim_h = test_entity->width + entity->height;
            
                        vec2f min_corner = -0.5f*_vec2f(dim_w, dim_h);
                        vec2f max_corner =  0.5f*_vec2f(dim_w, dim_h);
                
                        vec2f rel = entity->pos - test_entity->pos;

                        if (test_wall(min_corner.x, rel.x, rel.y, player_delta.x, player_delta.y,
                                      &t_min, min_corner.y, max_corner.y)) {
                            wall_normal = _vec2f(-1.0f, 0.0f);
                            hit_entity = test_entity;
                        }
                
                        if (test_wall(max_corner.x, rel.x, rel.y, player_delta.x, player_delta.y,
                                      &t_min, min_corner.y, max_corner.y)) {
                            wall_normal = _vec2f(1.0f, 0.0f);
                            hit_entity = test_entity;
                        }
                
                        if (test_wall(min_corner.y, rel.y, rel.x, player_delta.y, player_delta.x,
                                      &t_min, min_corner.x, max_corner.x)) {
                            wall_normal = _vec2f(0.0f, -1.0f);
                            hit_entity = test_entity;
                        }
                
                        if (test_wall(max_corner.y, rel.y, rel.x, player_delta.y, player_delta.x,
                                      &t_min, min_corner.x, max_corner.x)) {
                            wall_normal = _vec2f(0.0f, 1.0f);
                            hit_entity = test_entity;
                        }
                    }
                }
            }
        }
            
        entity->pos += t_min*player_delta;
        if (hit_entity) {
            entity->d_pos = entity->d_pos - 1*vec_dot(entity->d_pos, wall_normal)*wall_normal;
            player_delta = desired_pos - entity->pos;
            player_delta = player_delta - 1*vec_dot(player_delta, wall_normal)*wall_normal;

            // TODO(xkazu0x): stairs
            //entity->tile_z += hit_low_entity->simd_tile_z;
        } else {
            break;
        }
    }

    // NOTE(xkazu0x): update facing direction
    // TODO(xkazu0x): change to using the acceleration vector
    if ((entity->d_pos.x == 0.0f) && (entity->d_pos.y == 0.0f)) {
        // NOTE(xkazu0x): leave direction whatever it was
    } else if (abs_f32(entity->d_pos.x) > abs_f32(entity->d_pos.y)) {
        if (entity->d_pos.x > 0) {
            entity->direction = 1;
        } else {
            entity->direction = 3;
        }
    } else if (abs_f32(entity->d_pos.x) < abs_f32(entity->d_pos.y)) {
        if (entity->d_pos.y > 0) {
            entity->direction = 0;
        } else {
            entity->direction = 2;
        }
    }
}
