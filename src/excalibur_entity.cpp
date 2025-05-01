inline move_spec_t
default_move_spec(void) {
    move_spec_t result;

    result.unit_max_accel_vector = false;
    result.speed = 1.0f;
    result.drag= 0.0f;
    
    return(result);
}
