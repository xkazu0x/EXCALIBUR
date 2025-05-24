internal Sim_Entity_Hash *
get_hash_from_storage_index(Sim_Region *region, u32 storage_index) {
    Assert(storage_index);
    Sim_Entity_Hash *result = 0;
    
    u32 hash_value = storage_index;
    for (u32 offset = 0;
         offset < ArrayCount(region->hash);
         ++offset) {
        u32 hash_mask = (ArrayCount(region->hash) - 1);
        u32 hash_index = ((hash_value + offset) & (hash_mask));
        Sim_Entity_Hash *entry = region->hash + hash_index;
        if (entry->index == 0 || entry->index == storage_index) {
            result = entry;
            break;
        }
    }
    
    return(result);
}

inline Sim_Entity *
get_entity_by_storage_index(Sim_Region *region, u32 storage_index) {
    Sim_Entity_Hash *entry = get_hash_from_storage_index(region, storage_index);
    Sim_Entity *result = entry->ptr;
    return(result);
}

inline Vec3
get_sim_space_pos(Sim_Region *region, Low_Entity *stored) {
    // NOTE(xkazu0x): map the entity in camera space
    Vec3 result = INVALID_POS;
    
    if (!is_entity_flag_set(&stored->sim, EntityFlag_NonSpatial)) {
        result = subtract(region->world, &stored->pos, &region->origin);
    }
    
    return(result);
}

internal Sim_Entity *
add_sim_entity(Game_State *state, Sim_Region *region, u32 storage_index, Low_Entity *stored, Vec3 *pos);
inline void
load_entity_reference(Game_State *state, Sim_Region *region, Entity_Reference *reference) {
    if (reference->index) {
        Sim_Entity_Hash *entry = get_hash_from_storage_index(region, reference->index);
        if (entry->ptr == 0) {
            entry->index = reference->index;
            
            Low_Entity *low_entity = get_low_entity(state, reference->index);
            Vec3 pos = get_sim_space_pos(region, low_entity);
            
            entry->ptr = add_sim_entity(state, region, reference->index, low_entity, &pos);
        }
        reference->ptr = entry->ptr;
    }
}

inline void
store_entity_reference(Entity_Reference *reference) {
    if (reference->ptr != 0) {
        reference->index = reference->ptr->storage_index;
    }
}

internal Sim_Entity *
add_sim_entity_raw(Game_State *state, Sim_Region *region,
                   u32 storage_index, Low_Entity *stored) {
    Assert(storage_index);
    Sim_Entity *entity = 0;
    
    Sim_Entity_Hash *entry = get_hash_from_storage_index(region, storage_index);
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

                Assert(!is_entity_flag_set(&stored->sim, EntityFlag_Simming));
                add_entity_flags(&stored->sim, EntityFlag_Simming);
            }
        
            entity->storage_index = storage_index;
            entity->updatable = false;
        } else {
            InvalidPath;
        }
    }

    return(entity);
}

internal inline b32
entity_overlaps_rect(Vec3 pos, Sim_Entity_Collision_Volume volume, Rect3 rect) {
    Rect3 grown = rect_add_radius(rect, 0.5f*volume.dim);
    b32 result = is_in_rect(grown, pos + volume.offset);
    return(result);
}

internal Sim_Entity *
add_sim_entity(Game_State *state, Sim_Region *region, u32 storage_index, Low_Entity *stored, Vec3 *pos) {
    Sim_Entity *entity = add_sim_entity_raw(state, region, storage_index, stored);
    
    if (entity) {
        if (pos) {
            entity->pos = *pos;
            entity->updatable = entity_overlaps_rect(entity->pos, entity->collision->total_volume, region->updatable_bounds);
        } else {
            entity->pos = get_sim_space_pos(region, stored);
        }
    }
    
    return(entity);
}

internal Sim_Region *
begin_sim(Arena *sim_arena, Game_State *state, World *world,
          World_Position origin, Rect3 bounds, f32 delta_time) {
    // TODO(xkazu0x): if entities were stored in the world, we wouldn't need the game state here
    
    Sim_Region *region = push_struct(sim_arena, Sim_Region);
    zero_struct(region->hash);

    // TODO(xkazu0x): try to make these get enforced more rigoriously
    region->max_entity_radius = 5.0f;
    region->max_entity_velocity = 30.0f;
    f32 update_safety_margin = region->max_entity_radius + delta_time*region->max_entity_velocity;
    f32 update_safety_margin_z = 1.0f;
    
    region->world = world;
    region->origin = origin;
    region->updatable_bounds = rect_add_radius(bounds, make_vec3(region->max_entity_radius));
    region->bounds = rect_add_radius(region->updatable_bounds, make_vec3(update_safety_margin, update_safety_margin, update_safety_margin_z));
    
    // TODO(xkazu0x): need to be more specific about entity counts
    region->max_entity_count = 4096;
    region->entity_count = 0;
    region->entities = push_array(sim_arena, Sim_Entity, region->max_entity_count);
    
    World_Position min_chunk_pos = map_into_chunk_space(world, region->origin, get_rect_min(region->bounds));
    World_Position max_chunk_pos = map_into_chunk_space(world, region->origin, get_rect_max(region->bounds));
    
    for (s32 chunk_z = min_chunk_pos.chunk_z;
         chunk_z <= max_chunk_pos.chunk_z;
         ++chunk_z) {
        for (s32 chunk_y = min_chunk_pos.chunk_y;
             chunk_y <= max_chunk_pos.chunk_y;
             ++chunk_y) {
            for (s32 chunk_x = min_chunk_pos.chunk_x;
                 chunk_x <= max_chunk_pos.chunk_x;
                 ++chunk_x) {
                World_Chunk *chunk = get_world_chunk(world, chunk_x, chunk_y, chunk_z);
            
                if (chunk) {
                    for (World_Entity_Block *block = &chunk->entity_block;
                         block;
                         block = block->next) {
                        for (u32 entity_index = 0;
                             entity_index < block->entity_count;
                             entity_index++) {
                            u32 low_entity_index = block->low_entity_index[entity_index];
                            Low_Entity *low = state->low_entities + low_entity_index;
                        
                            if (!is_entity_flag_set(&low->sim, EntityFlag_NonSpatial)) {
                                Vec3 sim_space_pos = get_sim_space_pos(region, low);
                            
                                if (entity_overlaps_rect(sim_space_pos, low->sim.collision->total_volume, region->bounds)) {
                                    add_sim_entity(state, region, low_entity_index, low, &sim_space_pos);
                                }
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
end_sim(Sim_Region *region, Game_State *state) {
    // TODO(xkazu0x): maybe don't take a game state here, low entities
    // should be stored in the world??
    
    Sim_Entity *entity = region->entities;
    for (u32 entity_index = 0;
         entity_index < region->entity_count;
         ++entity_index, ++entity)
    {
        Low_Entity *stored = state->low_entities + entity->storage_index;

        Assert(is_entity_flag_set(&stored->sim, EntityFlag_Simming));
        stored->sim = *entity;
        Assert(!is_entity_flag_set(&stored->sim, EntityFlag_Simming));
        
        store_entity_reference(&stored->sim.sword);
        
        // TODO(xkazu0x): save stat back to the stored entity, once sim entities
        // do state decompression.
        World_Position new_pos = is_entity_flag_set(entity, EntityFlag_NonSpatial) ?
            null_position() :
            map_into_chunk_space(region->world, region->origin, entity->pos);
        change_entity_location(&state->world_arena, state->world, entity->storage_index,
                               stored, new_pos);

        if (entity->storage_index == state->camera_following_entity_index) {
            World_Position new_camera_pos = state->camera_pos;
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
            f32 camera_offset_z = new_camera_pos.offset_.z;
            new_camera_pos = stored->pos;
            new_camera_pos.offset_.z = camera_offset_z;
#endif
            state->camera_pos = new_camera_pos;
        }
    }
}

struct test_wall_t {
    f32 x;
    f32 rel_x;
    f32 rel_y;
    f32 delta_x;
    f32 delta_y;
    f32 min_y;
    f32 max_y;
    Vec3 normal;
};

internal b32
can_collide(Game_State *state, Sim_Entity *a, Sim_Entity *b) {
    b32 result = false;

    if (a->storage_index > b->storage_index) {
        Sim_Entity *temp = a;
        a = b;
        b = temp;
    }
    if (is_entity_flag_set(a, EntityFlag_Collides) &&
        is_entity_flag_set(b, EntityFlag_Collides)) {
        if (!is_entity_flag_set(a, EntityFlag_NonSpatial) &&
            !is_entity_flag_set(b, EntityFlag_NonSpatial)) {
            // TODO(xkazu0x): property-based logic goes here
            result = true;
        }

        // TODO(xkazu0x): BETTER HASH FUCTION
        u32 hash_bucket = a->storage_index & (ArrayCount(state->collision_rule_hash) - 1);
        for (Pairwise_Collision_Rule *rule = state->collision_rule_hash[hash_bucket];
             rule;
             rule = rule->next_in_hash)
        {
            if ((rule->storage_index_a == a->storage_index) &&
                (rule->storage_index_b == b->storage_index)) {
                result = rule->can_collide;
                break;
            }
        }
    }
    
    return(result);
}

internal b32
handle_collision(Game_State *state, Sim_Entity *a, Sim_Entity *b) {
    b32 stops_on_collision = false;

    if (a->type == EntityType_Sword) {
        add_collision_rule(state, a->storage_index, b->storage_index, false);
        stops_on_collision = false;
    } else {
        stops_on_collision = true;
    }
    
    if (a->type > b->type) {
        Sim_Entity *temp = a;
        a = b;
        b = temp;
    }
    
    if ((a->type == EntityType_Monster) &&
        (b->type == EntityType_Sword)) {
        if (a->hit_point_max == 1) {
            make_entity_non_spatial(a);
        } else {
            --a->hit_point_max;
        }
    }
    
    return(stops_on_collision);
}

internal b32
can_overlap(Game_State *state, Sim_Entity *mover, Sim_Entity *region) {
    b32 result = false;
    if (mover != region) {
        if (region->type == EntityType_Stairwell) {
            result = true;
        }
    }
    return(result);
}

internal void
handle_overlap(Game_State *state, Sim_Entity *mover, Sim_Entity *region,
               f32 delta_time, f32 *ground) {
    if (region->type == EntityType_Stairwell) {
        *ground = get_stair_ground(region, get_entity_ground_point(mover));
    }
}

internal b32
speculative_collide(Sim_Entity *mover, Sim_Entity *region, Vec3 test_pos) {
    b32 result = true;
    if (region->type == EntityType_Stairwell) {
        f32 step_height = 0.1f;
#if 0
        result = ((abs_f32(get_entity_ground_point(mover).z - ground) > step_height) ||
                  ((bary.y > 0.1f) && (bary.y < 0.9f)));
#endif
        Vec3 mover_ground_point = get_entity_ground_point(mover, test_pos);
        f32 ground = get_stair_ground(region, mover_ground_point);
        result = (abs_f32(mover_ground_point.z - ground) > step_height);
    }
    return(result);
}

internal b32
entities_overlap(Sim_Entity *entity, Sim_Entity *test_entity, Vec3 epsilon = make_vec3(0.0f)) {
    b32 result = false;
    
    for (u32 volume_index = 0;
         !result && (volume_index < entity->collision->volume_count);
         ++volume_index) {
        Sim_Entity_Collision_Volume *volume = entity->collision->volumes + volume_index;
                    
        for (u32 test_volume_index = 0;
             !result && (test_volume_index < test_entity->collision->volume_count);
             ++test_volume_index) {
            Sim_Entity_Collision_Volume *test_volume = test_entity->collision->volumes + test_volume_index;
                        
            Rect3 entity_rect = make_rect3_center_dim(entity->pos + volume->offset, volume->dim + epsilon);
            Rect3 test_entity_rect = make_rect3_center_dim(test_entity->pos + test_volume->offset, test_volume->dim);
                        
            result = rect_intersect(entity_rect, test_entity_rect);
        }
    }

    return(result);
}

internal void
move_entity(Game_State *state, Sim_Region *region, Sim_Entity *entity, f32 delta,
            Move_Spec *move_spec, Vec3 dd_pos) {
    Assert(!is_entity_flag_set(entity, EntityFlag_NonSpatial));
    
    if (move_spec->unit_max_accel_vector) {
        f32 dd_pos_length = length_squared(dd_pos);
        if (dd_pos_length > 1.0f) {
            dd_pos *= (1.0f/square_root(dd_pos_length));
        }
    }
    
    dd_pos *= move_spec->speed;
    
    // TODO(xkazu0x): ODE here!
    Vec3 drag = -move_spec->drag*entity->d_pos;
    drag.z = 0.0f;
    dd_pos += drag;
    if (!is_entity_flag_set(entity, EntityFlag_ZSupported)) {
        dd_pos += make_vec3(0.0f, 0.0f, -9.8f); // NOTE(xkazu0x): gravity
    }
    
    Vec3 player_delta = (0.5f*dd_pos*square(delta) + entity->d_pos*delta);
    entity->d_pos = dd_pos*delta + entity->d_pos;
    // TODO(xkazu0x): upgrade physical motion routines to handle capping the
    // maximum velocity?
    Assert(length_squared(entity->d_pos) <= square(region->max_entity_velocity));

    f32 distance_remaining = entity->distance_limit;
    if (distance_remaining == 0.0f) {
        // TODO(xkazu0x): do we want to formalize this number?
        distance_remaining = 10000.0f;
    }
    
    for (u32 iteration = 0;
         iteration < 4;
         ++iteration) {
        f32 t_min = 1.0f;
        f32 t_max = 0.0f;
        
        // TODO(xkazu0x): what do we want to do for epsilons here?
        // think this through for the final collision code
        f32 player_delta_length = length(player_delta);
        if (player_delta_length > 0.0f) {
            if (player_delta_length > distance_remaining) {
                t_min = (distance_remaining/player_delta_length);
            }
        
            Vec3 wall_normal_min = make_vec3(0.0f);
            Vec3 wall_normal_max = make_vec3(0.0f);
            
            Sim_Entity *hit_entity_min = 0;
            Sim_Entity *hit_entity_max = 0;

            Vec3 desired_pos = entity->pos + player_delta;

            // NOTE(xkazu0x): this is just an optimization to avoid enterring the
            // loop in the case where the test entity is non-spatial
            if (!is_entity_flag_set(entity, EntityFlag_NonSpatial))
            {
                // TODO(xkazu0x): spatial partition here!
                for (u32 test_entity_index = 0;
                     test_entity_index < region->entity_count;
                     ++test_entity_index)
                {
                    Sim_Entity *test_entity = region->entities + test_entity_index;
                    Vec3 overlap_epsilon = make_vec3(0.001f);
                    
                    if ((is_entity_flag_set(test_entity, EntityFlag_Traversable) &&
                         entities_overlap(entity, test_entity, overlap_epsilon)) ||
                        can_collide(state, entity, test_entity))
                    {
                        for (u32 volume_index = 0;
                             volume_index < entity->collision->volume_count;
                             ++volume_index)
                        {
                            Sim_Entity_Collision_Volume *volume =
                                entity->collision->volumes + volume_index;
                            
                            for (u32 test_volume_index = 0;
                                 test_volume_index < test_entity->collision->volume_count;
                                 ++test_volume_index)
                            {
                                Sim_Entity_Collision_Volume *test_volume =
                                    test_entity->collision->volumes + test_volume_index;
                                
                                Vec3 minkowski_diameter = test_volume->dim + volume->dim;
                                Vec3 min_corner = -0.5f*minkowski_diameter;
                                Vec3 max_corner =  0.5f*minkowski_diameter;
                                
                                Vec3 rel = ((entity->pos + volume->offset) - (test_entity->pos + test_volume->offset));
                                
                                if ((rel.z >= min_corner.z) && (rel.z < max_corner.z))
                                {
                                    test_wall_t walls[] = {
                                        {min_corner.x, rel.x, rel.y, player_delta.x, player_delta.y, min_corner.y, max_corner.y, make_vec3(-1.0f,  0.0f, 0.0f)},
                                        {max_corner.x, rel.x, rel.y, player_delta.x, player_delta.y, min_corner.y, max_corner.y, make_vec3( 1.0f,  0.0f, 0.0f)},
                                        {min_corner.y, rel.y, rel.x, player_delta.y, player_delta.x, min_corner.x, max_corner.x, make_vec3( 0.0f, -1.0f, 0.0f)},
                                        {max_corner.y, rel.y, rel.x, player_delta.y, player_delta.x, min_corner.x, max_corner.x, make_vec3( 0.0f,  1.0f, 0.0f)},
                                    };
                                    
                                    if (is_entity_flag_set(test_entity, EntityFlag_Traversable)) {
                                        f32 t_max_test = t_max;
                                        Vec3 test_wall_normal = make_vec3(0.0f);
                                        b32 hit_this = false;
                                            
                                        for (u32 wall_index = 0;
                                             wall_index < ArrayCount(walls);
                                             ++wall_index)
                                        {
                                            test_wall_t *wall = walls + wall_index;
                                            
                                            f32 t_epsilon = 0.001f;
                                            if (wall->delta_x != 0.0f)
                                            {
                                                f32 t_result = (wall->x - wall->rel_x) / wall->delta_x;
                                                f32 y = wall->rel_y + t_result*wall->delta_y;
                                                
                                                if ((t_result >= 0.0f) && (t_max_test < t_result))
                                                {
                                                    if (y >= wall->min_y && (y <= wall->max_y))
                                                    {
                                                        t_max_test = Max(0.0f, t_result - t_epsilon);
                                                        test_wall_normal = wall->normal;
                                                        hit_this = true;
                                                    }
                                                }
                                            }
                                        }
                                            
                                        if (hit_this) {
                                            t_max = t_max_test;
                                            wall_normal_max = test_wall_normal;
                                            hit_entity_max = test_entity;
                                        }
                                    } else {
                                        f32 t_min_test = t_min;
                                        b32 hit_this = false;

                                        Vec3 test_wall_normal = make_vec3(0.0f);
                                        
                                        for (u32 wall_index = 0;
                                             wall_index < ArrayCount(walls);
                                             ++wall_index) {
                                            test_wall_t *wall = walls + wall_index;
                                            
                                            f32 t_epsilon = 0.001f;
                                            if (wall->delta_x != 0.0f) {
                                                f32 t_result = (wall->x - wall->rel_x) / wall->delta_x;
                                                f32 y = wall->rel_y + t_result*wall->delta_y;
                                                if ((t_result >= 0.0f) && (t_min_test > t_result)) {
                                                    if (y >= wall->min_y && (y <= wall->max_y)) {
                                                        t_min_test = Max(0.0f, t_result - t_epsilon);
                                                        test_wall_normal = wall->normal;
                                                        hit_this = true;
                                                    }
                                                }
                                            }
                                        }
                                        
                                        if (hit_this) {
                                            Vec3 test_pos = entity->pos + t_min_test*player_delta;
                                            if (speculative_collide(entity, test_entity, test_pos)) {
                                                t_min = t_min_test;
                                                wall_normal_min = test_wall_normal;
                                                hit_entity_min = test_entity;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            
            Vec3 wall_normal;
            Sim_Entity *hit_entity;
            f32 t_stop;
            
            if (t_min < t_max) {
                t_stop = t_min;
                hit_entity = hit_entity_min;
                wall_normal = wall_normal_min;
            } else {
                t_stop = t_max;
                hit_entity = hit_entity_max;
                wall_normal = wall_normal_max;
            }
            
            entity->pos += t_stop*player_delta;
            distance_remaining -= t_stop*player_delta_length;

            if (hit_entity) {
                player_delta = desired_pos - entity->pos;

                b32 stops_on_collision = handle_collision(state, entity, hit_entity);
                if (stops_on_collision) {
                    player_delta = player_delta - 1*dot_product(player_delta, wall_normal)*wall_normal;
                    entity->d_pos = entity->d_pos - 1*dot_product(entity->d_pos, wall_normal)*wall_normal;
                }
            } else {
                break;
            }
        } else {
            break;
        }
    }

    f32 ground = 0.0f;

    // NOTE(xkazu0x): handle events based on area overlapping 
    // TODO(xkazu0x): handle overlapping precisely by moving it into the collision loop?
    {
        // TODO(xkazu0x): spatial partition here!
        for (u32 test_entity_index = 0;
             test_entity_index < region->entity_count;
             ++test_entity_index) {
            Sim_Entity *test_entity = region->entities + test_entity_index;
            
            if (can_overlap(state, entity, test_entity) &&
                entities_overlap(entity, test_entity)) {
                handle_overlap(state, entity, test_entity, delta, &ground);
            }
        }
    }
    
    ground += entity->pos.z - get_entity_ground_point(entity).z;
    if ((entity->pos.z <= ground) ||
        (is_entity_flag_set(entity, EntityFlag_ZSupported) &&
         (entity->d_pos.z == 0.0f))) {
        entity->pos.z = ground;
        entity->d_pos.z = 0.0f;
        add_entity_flags(entity, EntityFlag_ZSupported);
    } else {
        clear_entity_flags(entity, EntityFlag_ZSupported);
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
