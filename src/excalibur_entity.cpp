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
}
