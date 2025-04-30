internal sim_entity_hash_t *
get_hash_from_storage_index(sim_region_t *region, u32 storage_index) {
    ASSERT(storage_index);
    sim_entity_hash_t *result = 0;
    
    u32 hash_value = storage_index;
    for (u32 offset = 0;
         offset < ARRAY_COUNT(region->hash);
         ++offset) {
        u32 hash_mask = (ARRAY_COUNT(region->hash) - 1);
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
add_sim_entity(game_state_t *state, sim_region_t *region, u32 storage_index, low_entity_t *stored, vec2 *pos);

inline vec2
get_sim_space_pos(sim_region_t *region, low_entity_t *stored) {
    // NOTE(xkazu0x): map the entity in camera space
    vec2 result = INVALID_POS;
    
    if (!is_entity_flag_set(&stored->sim, ENTITY_FLAG_NON_SPATIAL)) {
        vec3 diff = subtract(region->world, &stored->pos, &region->origin);
        result = make_vec2(diff.x, diff.y);
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
            vec2 pos = get_sim_space_pos(region, low_entity);
            
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
    ASSERT(storage_index);
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

                ASSERT(!is_entity_flag_set(&stored->sim, ENTITY_FLAG_SIMMING));
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
add_sim_entity(game_state_t *state, sim_region_t *region, u32 storage_index, low_entity_t *stored, vec2 *pos) {
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
begin_sim(memory_arena_t *sim_arena, game_state_t *state, world_t *world, world_position_t origin, rect2 bounds) {
    // TODO(xkazu0x): if entities were stored in the world, we wouldn't need the game state here
    
    sim_region_t *region = (sim_region_t *)memory_arena_push(sim_arena, sizeof(sim_region_t));
    zero_struct(region->hash);

    // TODO(xkazu0x): IMPORTANT(xkazu0x): calculate this eventually from the maximun value
    // of all entities radius plus their speed
    f32 update_safety_margin = 1.0f;
    
    region->world = world;
    region->origin = origin;
    region->updatable_bounds = bounds;
    region->bounds = rect_add_radius(region->updatable_bounds, update_safety_margin, update_safety_margin);
    
    // TODO(xkazu0x): need to be more specific about entity counts
    region->max_entity_count = 4096;
    region->entity_count = 0;
    region->entities = (sim_entity_t *)memory_arena_push(sim_arena, region->max_entity_count*sizeof(sim_entity_t));
    
    world_position_t min_chunk_pos = map_into_chunk_space(world, region->origin, get_rect_min(region->bounds));
    world_position_t max_chunk_pos = map_into_chunk_space(world, region->origin, get_rect_max(region->bounds));
    
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
                            vec2 sim_space_pos = get_sim_space_pos(region, low);
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

        ASSERT(is_entity_flag_set(&stored->sim, ENTITY_FLAG_SIMMING));
        stored->sim = *entity;
        ASSERT(!is_entity_flag_set(&stored->sim, ENTITY_FLAG_SIMMING));
        
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
    b32 hit = false;
    
    f32 t_epsilon = 0.01f;
    if (player_delta_x != 0.0f) {
        f32 t_result = (wall_x - rel_x) / player_delta_x;
        f32 y = rel_y + t_result*player_delta_y;
        if ((t_result >= 0.0f) && (*t_min > t_result)) {
            if (y >= min_y && (y <= max_y)) {
                *t_min = MAX(0.0f, t_result - t_epsilon);
                hit = true;
            }
        }
    }
    
    return(hit);
}

internal b32
should_collide(game_state_t *state, sim_entity_t *a, sim_entity_t *b) {
    b32 result = false;

    if (a->storage_index > b->storage_index) {
        sim_entity_t *temp = a;
        a = b;
        b = temp;
    }
    
    if (!is_entity_flag_set(a, ENTITY_FLAG_NON_SPATIAL) &&
        !is_entity_flag_set(b, ENTITY_FLAG_NON_SPATIAL)) {
        // TODO(xkazu0x): property-based logic goes here
        result = true;
    }

    // TODO(xkazu0x): BETTER HASH FUCTION
    u32 hash_bucket = a->storage_index & (ARRAY_COUNT(state->collision_rule_hash) - 1);
    for (pairwise_collision_rule_t *rule = state->collision_rule_hash[hash_bucket];
         rule;
         rule = rule->next_in_hash)
    {
        if ((rule->storage_index_a == a->storage_index) &&
            (rule->storage_index_b == b->storage_index)) {
            result = rule->should_collide;
            break;
        }
    }
    
    return(result);
}

internal b32
handle_collision(sim_entity_t *a, sim_entity_t *b) {
    b32 stops_on_collision = false;

    if (a->type == ENTITY_TYPE_SWORD) {
        stops_on_collision = false;
    } else {
        stops_on_collision = true;
    }
    
    if (a->type > b->type) {
        sim_entity_t *temp = a;
        a = b;
        b = temp;
    }
    
    if ((a->type == ENTITY_TYPE_MONSTER) &&
        (b->type == ENTITY_TYPE_SWORD)) {
        if (a->hit_point_max == 1) {
            make_entity_non_spatial(a);
        } else {
            --a->hit_point_max;
        }
    }
    
    // TODO(xkazu0x): stairs
    //entity->tile_z += hit_low_entity->simd_tile_z;

    // TODO(xkazu0x): real "stops on collision"
    return(stops_on_collision);
}

internal void
move_entity(game_state_t *state, sim_region_t *region, sim_entity_t *entity, f32 delta,
            move_spec_t *move_spec, vec2 dd_pos) {
    ASSERT(!is_entity_flag_set(entity, ENTITY_FLAG_NON_SPATIAL));
    
    world_t *world = region->world;

    if (move_spec->unit_max_accel_vector) {
        f32 dd_pos_length = vec_length_square(dd_pos);
        if (dd_pos_length > 1.0f) {
            dd_pos *= (1.0f/square_root(dd_pos_length));
        }
    }
    
    dd_pos *= move_spec->speed;
    
    // TODO(xkazu0x): ODE here!
    dd_pos += -move_spec->drag*entity->d_pos;
    
    //vec2 old_player_pos = entity->pos;
    vec2 player_delta = (0.5f*dd_pos*square(delta) + entity->d_pos*delta);
    entity->d_pos = dd_pos*delta + entity->d_pos;
    //vec2 new_player_pos = old_player_pos + player_delta;

    f32 dd_z = -9.8f;
    entity->z = 0.5f*dd_z*square(delta) + entity->d_z*delta + entity->z;
    entity->d_z = dd_z*delta + entity->d_z;
    if (entity->z < 0.0f) entity->z = 0.0f;

    f32 distance_remaining = entity->distance_limit;
    if (distance_remaining == 0.0f) {
        // TODO(xkazu0x): do we want to formalize this number?
        distance_remaining = 10000.0f;
    }

    for (u32 iteration = 0;
         iteration < 4;
         ++iteration)
    {
        f32 t_min = 1.0f;
        
        f32 player_delta_length = vec_length(player_delta);
        // TODO(xkazu0x): what do we want to do for epsilons here?
        // think this through for the final collision code
        if (player_delta_length > 0.0f) {
            if (player_delta_length > distance_remaining) {
                t_min = (distance_remaining/player_delta_length);
            }
        
            vec2 wall_normal = {};
            sim_entity_t *hit_entity = 0;

            vec2 desired_pos = entity->pos + player_delta;

            // NOTE(xkazu0x): this is just an optimization to avoid enterring the
            // loop in the case where the test entity is non-spatial
            if (!is_entity_flag_set(entity, ENTITY_FLAG_NON_SPATIAL)) {
                // TODO(xkazu0x): spatial partition here!
                for (u32 test_high_entity_index = 0;
                     test_high_entity_index < region->entity_count;
                     ++test_high_entity_index)
                {
                    sim_entity_t *test_entity = region->entities + test_high_entity_index;
                    if (should_collide(state, entity, test_entity)) {
                        f32 dim_w = test_entity->width + entity->width;
                        f32 dim_h = test_entity->width + entity->height;
            
                        vec2 min_corner = -0.5f*make_vec2(dim_w, dim_h);
                        vec2 max_corner =  0.5f*make_vec2(dim_w, dim_h);
                
                        vec2 rel = entity->pos - test_entity->pos;

                        if (test_wall(min_corner.x, rel.x, rel.y, player_delta.x, player_delta.y,
                                      &t_min, min_corner.y, max_corner.y)) {
                            wall_normal = make_vec2(-1.0f, 0.0f);
                            hit_entity = test_entity;
                        }
                
                        if (test_wall(max_corner.x, rel.x, rel.y, player_delta.x, player_delta.y,
                                      &t_min, min_corner.y, max_corner.y)) {
                            wall_normal = make_vec2(1.0f, 0.0f);
                            hit_entity = test_entity;
                        }
                
                        if (test_wall(min_corner.y, rel.y, rel.x, player_delta.y, player_delta.x,
                                      &t_min, min_corner.x, max_corner.x)) {
                            wall_normal = make_vec2(0.0f, -1.0f);
                            hit_entity = test_entity;
                        }
                
                        if (test_wall(max_corner.y, rel.y, rel.x, player_delta.y, player_delta.x,
                                      &t_min, min_corner.x, max_corner.x)) {
                            wall_normal = make_vec2(0.0f, 1.0f);
                            hit_entity = test_entity;
                        }
                    }
                }
            }
            
            entity->pos += t_min*player_delta;
            distance_remaining -= t_min*player_delta_length;

            if (hit_entity) {
                player_delta = desired_pos - entity->pos;
                
                b32 stops_on_collision = handle_collision(entity, hit_entity);
                if (stops_on_collision) {
                    player_delta = player_delta - 1*vec_dot(player_delta, wall_normal)*wall_normal;
                    entity->d_pos = entity->d_pos - 1*vec_dot(entity->d_pos, wall_normal)*wall_normal;
                } else {
                    add_collision_rule(state, entity->storage_index, hit_entity->storage_index, false);
                }
            } else {
                break;
            }
        } else {
            break;
        }
    }

    if (entity->distance_limit != 0.0f) {
        entity->distance_limit = distance_remaining;
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
