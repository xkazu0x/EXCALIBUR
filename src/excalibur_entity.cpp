inline Move_Spec
default_move_spec(void) {
    Move_Spec result;
    result.unit_max_accel_vector = false;
    result.speed = 1.0f;
    result.drag= 0.0f;
    return(result);
}
