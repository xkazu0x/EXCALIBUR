internal Render_Group *
alloc_render_group(Arena *arena, u32 max_push_buffer_size, f32 meters_to_pixels) {
    Render_Group *result = push_struct(arena, Render_Group);
    result->push_buffer_base = (u8 *)push_size(arena, max_push_buffer_size);
    
    result->default_basis = push_struct(arena, Render_Basis);
    result->default_basis->pos = make_vec3(0.0f);
    result->meters_to_pixels = meters_to_pixels;
    result->piece_count = 0;
    
    result->max_push_buffer_size = max_push_buffer_size;
    result->push_buffer_size = 0;
    
    return(result);
}
