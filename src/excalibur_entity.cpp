inline move_spec_t
default_move_spec(void) {
    move_spec_t result;

    result.unit_max_accel_vector = EX_FALSE;
    result.speed = 1.0f;
    result.drag= 0.0f;
    
    return(result);
}

internal void
update_familiar(sim_region_t *sim_region, sim_entity_t *entity, f32 delta) {
    sim_entity_t *closest_player = 0;
    f32 closest_player_distance_sqr = sqr(10.0f); // NOTE(xkazu0x): ten meter maximun search!

    // TODO(xkazu0x): make spatial queries easy for things!
    sim_entity_t *test_entity = sim_region->entities;
    for (u32 test_entity_index = 0;
         test_entity_index < sim_region->entity_count;
         ++test_entity_index, ++test_entity) {
        if (test_entity->type == ENTITY_TYPE_PLAYER) {
            f32 test_distance_sqr = vec_length_sqr(test_entity->pos - entity->pos);
            if (closest_player_distance_sqr > test_distance_sqr) {
                closest_player = test_entity;
                closest_player_distance_sqr = test_distance_sqr;
            }
        }
    }

    vec2f dd_pos = _vec2f(0.0f);
    if ((closest_player) && (closest_player_distance_sqr > sqr(2.5f))) {
        f32 acceleration = 0.5f;
        f32 lenght_normalized = acceleration / sqrt_f32(closest_player_distance_sqr);
        dd_pos = lenght_normalized*(closest_player->pos - entity->pos);
    }

    move_spec_t move_spec = default_move_spec();
    move_spec.unit_max_accel_vector = EX_TRUE;
    move_spec.speed = 50.0f;
    move_spec.drag = 5.0f;
    move_entity(sim_region, entity, delta, &move_spec, dd_pos);
    
    if (dd_pos.x == 0.0f && dd_pos.y == 0.0f) {
        entity->direction = 2;
    }
}

internal void
update_monster(sim_region_t *sim_region, sim_entity_t *entity, f32 delta) {
}

internal void
update_sword(sim_region_t *sim_region, sim_entity_t *entity, f32 delta) {
    move_spec_t move_spec = default_move_spec();
    move_spec.unit_max_accel_vector = EX_FALSE;
    move_spec.speed = 0.0f;
    move_spec.drag = 0.0f;

    vec2f old_pos = entity->pos;
    move_entity(sim_region, entity, delta, &move_spec, _vec2f(0.0f));
    f32 distance_traveled = vec_length(entity->pos - old_pos);
    
    entity->distance_remaining -= distance_traveled;
    if (entity->distance_remaining < 0.0f) {
        EX_ASSERT(!"NEED TO MAKE ENTITIES BE ABLE TO NOT BE THERE!!!");
    }
}
