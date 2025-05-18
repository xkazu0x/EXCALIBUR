internal void
draw_rect(Bitmap *buffer, Vec2 min, Vec2 max, Vec3 color, f32 a) {
    s32 min_x = round_f32_to_s32(min.x);
    s32 min_y = round_f32_to_s32(min.y);
    
    s32 max_x = round_f32_to_s32(max.x);
    s32 max_y = round_f32_to_s32(max.y);

    if (min_x < 0) min_x = 0;
    if (min_y < 0) min_y = 0;
    
    if (max_x > buffer->width) max_x = buffer->width;
    if (max_y > buffer->height) max_y = buffer->height;

    u32 out_color = ((round_f32_to_u32(a*255.0f) << 24) |
                     (round_f32_to_u32(color.r*255.0f) << 16) |
                     (round_f32_to_u32(color.g*255.0f) << 8) |
                     (round_f32_to_u32(color.b*255.0f) << 0));
    
    u8 *row = ((u8 *)buffer->memory +
               (min_x*BYTES_PER_PIXEL) +
               (min_y*buffer->pitch));
    for (s32 y = min_y; y < max_y; ++y) {
        u32 *pixel = (u32 *)row;
        for (s32 x = min_x; x < max_x; ++x) {
            *pixel++ = out_color;
        }
        row += buffer->pitch;
    }
}

internal void
draw_rect_outline(Bitmap *buffer, Vec2 min, Vec2 max, Vec3 color, f32 r) {
    // NOTE(xkazu0x): top and bottom
    draw_rect(buffer,
              make_vec2(min.x - r, min.y - r),
              make_vec2(max.x + r, min.y + r),
              color);
    draw_rect(buffer,
              make_vec2(min.x - r, max.y - r),
              make_vec2(max.x + r, max.y + r),
              color);
    
    // NOTE(xkazu0x): left and right
    draw_rect(buffer,
              make_vec2(min.x - r, min.y - r),
              make_vec2(min.x + r, max.y + r),
              color);
    draw_rect(buffer,
              make_vec2(max.x - r, min.y - r),
              make_vec2(max.x + r, max.y + r),
              color);
}

internal void
draw_bitmap(Bitmap *buffer, Bitmap *bitmap, Vec2 offset, f32 c_alpha) {
    s32 min_x = round_f32_to_s32(offset.x);
    s32 min_y = round_f32_to_s32(offset.y);
    s32 max_x = min_x + bitmap->width;
    s32 max_y = min_y + bitmap->height;

    s32 source_offset_x = 0;
    s32 source_offset_y = 0;
    
    if (min_x < 0) {
        source_offset_x = -min_x;
        min_x = 0;
    }
    if (min_y < 0) {
        source_offset_y = -min_y;
        min_y = 0;
    }
    
    if (max_x > buffer->width) max_x = buffer->width;
    if (max_y > buffer->height) max_y = buffer->height;

    // TODO(xkazu0x): source_row needs to be changed based on cliping
    u8 *source_row = (u8 *)bitmap->memory + source_offset_y*bitmap->pitch + BYTES_PER_PIXEL*source_offset_x;
    u8 *dest_row = ((u8 *)buffer->memory +
                    min_x*BYTES_PER_PIXEL +
                    min_y*buffer->pitch);
    
    for (s32 y = min_y; y < max_y; ++y) {
        u32 *dest   = (u32 *)dest_row;
        u32 *source = (u32 *)source_row;
        
        for (s32 x = min_x; x < max_x; ++x) {
            f32 src_a = (f32)((*source >> 24) & 0xFF);
            f32 src_r = c_alpha*(f32)((*source >> 16) & 0xFF);
            f32 src_g = c_alpha*(f32)((*source >> 8) & 0xFF);
            f32 src_b = c_alpha*(f32)((*source >> 0) & 0xFF);
            f32 real_src_a = (src_a/255.0f)*c_alpha;
            
            f32 dest_a = (f32)((*dest >> 24) & 0xFF);
            f32 dest_r = (f32)((*dest >> 16) & 0xFF);
            f32 dest_g = (f32)((*dest >> 8) & 0xFF);
            f32 dest_b = (f32)((*dest >> 0) & 0xFF);
            f32 real_dest_a = (dest_a/255.0f);

            f32 inv_real_src_a = (1.0f - real_src_a);
            f32 a = 255.0f*(real_src_a + real_dest_a - real_src_a*real_dest_a);
            f32 r = inv_real_src_a*dest_r + src_r;
            f32 g = inv_real_src_a*dest_g + src_g;
            f32 b = inv_real_src_a*dest_b + src_b;

            *dest = (((u32)(a + 0.5f) << 24) |
                     ((u32)(r + 0.5f) << 16) |
                     ((u32)(g + 0.5f) << 8) |
                     ((u32)(b + 0.5f) << 0));
            
            ++dest;
            ++source;
        }
        
        dest_row += buffer->pitch;
        source_row += bitmap->pitch;
    }
}

internal Vec2
get_render_entity_basis_point(Render_Group *group, Render_Entity_Basis *entity_basis, Vec2 screen_center) {
    Vec3 entity_base_pos = entity_basis->basis->pos;
    f32 fudge_z = (1.0f + 0.1f*entity_base_pos.z + entity_basis->offset.z);
            
    f32 entity_ground_point_x = screen_center.x + group->meters_to_pixels*fudge_z*entity_base_pos.x;
    f32 entity_ground_point_y = screen_center.y - group->meters_to_pixels*fudge_z*entity_base_pos.y;
    f32 entity_z = -group->meters_to_pixels*entity_base_pos.z;
                
    Vec2 center = make_vec2(entity_ground_point_x + entity_basis->offset.x,
                            entity_ground_point_y + entity_basis->offset.y + entity_basis->entity_zc*entity_z);
    return(center);
}

internal Render_Group *
render_group_alloc(Arena *arena, u32 max_push_buffer_size, f32 meters_to_pixels) {
    Render_Group *result = push_struct(arena, Render_Group);
    result->push_buffer_base = (u8 *)push_size(arena, max_push_buffer_size);
    
    result->default_basis = push_struct(arena, Render_Basis);
    result->default_basis->pos = make_vec3(0.0f);
    result->meters_to_pixels = meters_to_pixels;
    
    result->max_push_buffer_size = max_push_buffer_size;
    result->push_buffer_size = 0;
    
    return(result);
}

internal void
render_group_draw(Render_Group *group, Bitmap *output_target) {
    Vec2 screen_center = 0.5f*make_vec2((f32)output_target->width, (f32)output_target->height);
        
    for (u32 base_address = 0;
         base_address < group->push_buffer_size;
         ) {
        Render_Entry_Header *header = (Render_Entry_Header *)(group->push_buffer_base + base_address);
        switch (header->type) {
            case RenderEntryType_Render_Entry_Clear: {
                Render_Entry_Clear *entry = (Render_Entry_Clear *)header;
                {
                    draw_rect(output_target,
                              make_vec2(0.0f),
                              make_vec2((f32)output_target->width, (f32)output_target->height),
                              entry->color.rgb, entry->color.a);
                }
                base_address += sizeof(*entry);
            } break;
                
            case RenderEntryType_Render_Entry_Rect: {
                Render_Entry_Rect *entry = (Render_Entry_Rect *)header;
                {
                    Vec2 point = get_render_entity_basis_point(group, &entry->entity_basis, screen_center);
                    draw_rect(output_target, point, point + entry->dim, entry->color.rgb);
                }
                base_address += sizeof(*entry);
            } break;
                
            case RenderEntryType_Render_Entry_Bitmap: {
                Render_Entry_Bitmap *entry = (Render_Entry_Bitmap *)header;
                {
                    Assert(entry->bitmap);
                    Vec2 point = get_render_entity_basis_point(group, &entry->entity_basis, screen_center);
                    draw_bitmap(output_target, entry->bitmap, point, entry->color.a);
                }
                base_address += sizeof(*entry);
            } break;

            InvalidDefaultCase;
        }
    }
}

internal Render_Entry_Header *
render_push_(Render_Group *group, u32 size, Render_Entry_Type type) {
    Render_Entry_Header *result = 0;
    if ((group->push_buffer_size + size) < group->max_push_buffer_size) {
        result = (Render_Entry_Header *)(group->push_buffer_base + group->push_buffer_size);
        result->type = type;
        group->push_buffer_size += size;
    } else {
        InvalidPath;
    }
    return(result);
}

internal void
render_clear(Render_Group *group, Vec4 color) {
    Render_Entry_Clear *entry = render_push(group, Render_Entry_Clear);
    if (entry) {
        entry->color = color;
    }
}

internal void
render_rect(Render_Group *group, Vec3 offset, Vec2 dim, Vec4 color, f32 entity_zc) {
    Render_Entry_Rect *entry = render_push(group, Render_Entry_Rect);
    if (entry) {
        Vec2 half_dim = 0.5f*group->meters_to_pixels*dim;
        entry->entity_basis.basis = group->default_basis;
        entry->entity_basis.offset = make_vec3((group->meters_to_pixels*make_vec2(offset.x, (-offset.y)) - half_dim),
                                               offset.z);
        entry->entity_basis.entity_zc = entity_zc;
        
        entry->dim = group->meters_to_pixels*dim;
        entry->color = color;
    }
}

internal void
render_rect_outline(Render_Group *group, Vec3 offset, Vec2 dim, Vec4 color, f32 entity_zc) {
    f32 thickness = 0.2f;
    
    // NOTE(xkazu0x): top and bottom
    render_rect(group, offset - make_vec3(0.0f, 0.5f*dim.y, 0.0f), make_vec2(dim.x, thickness), color, entity_zc);
    render_rect(group, offset + make_vec3(0.0f, 0.5f*dim.y, 0.0f), make_vec2(dim.x, thickness), color, entity_zc);
    
    // NOTE(xkazu0x): left and right
    render_rect(group, offset - make_vec3(0.5f*dim.x, 0.0f, 0.0f), make_vec2(thickness, dim.y), color, entity_zc);
    render_rect(group, offset + make_vec3(0.5f*dim.x, 0.0f, 0.0f), make_vec2(thickness, dim.y), color, entity_zc);
}

internal void
render_bitmap(Render_Group *group, Bitmap *bitmap, Vec3 offset, Vec2 align, f32 alpha, f32 entity_zc) {
    Render_Entry_Bitmap *entry = render_push(group, Render_Entry_Bitmap);
    if (entry) {
        entry->entity_basis.basis = group->default_basis;
        entry->entity_basis.offset = make_vec3(group->meters_to_pixels*offset.x - align.x,
                                               group->meters_to_pixels*(-offset.y) - align.y,
                                               offset.z);
        entry->entity_basis.entity_zc = entity_zc;
        entry->bitmap = bitmap;
        entry->color = make_vec4(make_vec3(1.0f), alpha);
    }
}
