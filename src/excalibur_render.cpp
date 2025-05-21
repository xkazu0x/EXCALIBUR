internal Vec4
srgb255_to_linear1(Vec4 color) {
    Vec4 result;

    f32 inv255 = 1.0f/255.0f;
    
    result.r = square(inv255*color.r);
    result.g = square(inv255*color.g);
    result.b = square(inv255*color.b);
    result.a = inv255*color.a;
    
    return(result);
}

internal Vec4
linear1_to_srgb255(Vec4 color) {
    Vec4 result;
    
    f32 one255 = 255.0f;
    
    result.r = one255*square_root(color.r);
    result.g = one255*square_root(color.g);
    result.b = one255*square_root(color.b);
    result.a = one255*color.a;
    
    return(result);
}

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
draw_rect_slowly(Bitmap *buffer, Vec2 origin, Vec2 axis_x, Vec2 axis_y,
                 Vec4 color, Bitmap *texture) {
    f32 inv_x_axis_length_squared = 1.0f/length_squared(axis_x);
    f32 inv_y_axis_length_squared = 1.0f/length_squared(axis_y);

    Vec2 points[4] = {
        origin,
        origin + axis_x,
        origin + axis_x + axis_y,
        origin + axis_y,
    };

    s32 width_max = (buffer->width - 1);
    s32 height_max = (buffer->height - 1);
    
    s32 min_x = width_max;
    s32 max_x = 0;
    
    s32 min_y = height_max;
    s32 max_y = 0;

    for (s32 point_index = 0;
         point_index < ArrayCount(points);
         ++point_index) {
        Vec2 test_point = points[point_index];
        
        s32 floor_x = (s32)floor_f32(test_point.x);
        s32 ceil_x = (s32)ceil_f32(test_point.x);
        
        s32 floor_y = (s32)floor_f32(test_point.y);
        s32 ceil_y = (s32)ceil_f32(test_point.y);

        if (min_x > floor_x) min_x = floor_x;
        if (min_y > floor_y) min_y = floor_y;
        
        if (max_x < ceil_x) max_x = ceil_x;
        if (max_y < ceil_y) max_y = ceil_y;
    }

    if (min_x < 0 ) min_x = 0;
    if (min_y < 0 ) min_y = 0;
    
    if (max_x > width_max) max_x = width_max;
    if (max_y > height_max) max_y = height_max;
    
    u8 *row = ((u8 *)buffer->memory +
               (min_x*BYTES_PER_PIXEL) +
               (min_y*buffer->pitch));
    for (s32 y = min_y;
         y <= max_y;
         ++y) {
        u32 *pixel = (u32 *)row;
        for (s32 x = min_x;
             x <= max_x;
             ++x) {
#if 1
            Vec2 pixel_point = make_vec2((f32)x, (f32)y);
            Vec2 d = pixel_point - origin;

            // TODO(xkazu0x): perp_dot()
            // TODO(xkazu0x): Simpler origin
            f32 edge0 = dot_product(d, -perp(axis_x));
            f32 edge1 = dot_product(d - axis_x, -perp(axis_y));
            f32 edge2 = dot_product(d - axis_x - axis_y, perp(axis_x));
            f32 edge3 = dot_product(d - axis_y, perp(axis_y));
           
            if ((edge0 < 0) &&
                (edge1 < 0) &&
                (edge2 < 0) &&
                (edge3 < 0)) {
                f32 u = inv_x_axis_length_squared*dot_product(d, axis_x);
                f32 v = inv_y_axis_length_squared*dot_product(d, axis_y);

                // TODO(xkazu0x):  SSE clamping
                //Assert((u >= 0.0f) && (u <= 1.0f));
                //Assert((v >= 0.0f) && (v <= 1.0f));

                // TODO(xkazu0x): formalize texture boundaries
                f32 f32_texture_x = ((u*(f32)(texture->width - 2)));
                f32 f32_texture_y = ((v*(f32)(texture->height - 2)));
                
                s32 s32_texture_x = (s32)f32_texture_x;
                s32 s32_texture_y = (s32)f32_texture_y;

                f32 f32_texture_rel_x = f32_texture_x - (f32)s32_texture_x;
                f32 f32_texture_rel_y = f32_texture_y - (f32)s32_texture_y;

                Assert((s32_texture_x >= 0) && (s32_texture_x < texture->width));
                Assert((s32_texture_y >= 0) && (s32_texture_y < texture->height));
                
                u8 *texel_ptr = ((u8 *)texture->memory) + s32_texture_x*BYTES_PER_PIXEL + s32_texture_y*texture->pitch;
                u32 texel_ptr_a = *(u32 *)(texel_ptr);
                u32 texel_ptr_b = *(u32 *)(texel_ptr + BYTES_PER_PIXEL);
                u32 texel_ptr_c = *(u32 *)(texel_ptr + texture->pitch);
                u32 texel_ptr_d = *(u32 *)(texel_ptr + texture->pitch + BYTES_PER_PIXEL);

                // TODO(xkazu0x): color.a
                Vec4 texel_a = make_vec4((f32)((texel_ptr_a >> 16) & 0xFF),
                                         (f32)((texel_ptr_a >> 8) & 0xFF),
                                         (f32)((texel_ptr_a >> 0) & 0xFF),
                                         (f32)((texel_ptr_a >> 24) & 0xFF));
                
                Vec4 texel_b = make_vec4((f32)((texel_ptr_b >> 16) & 0xFF),
                                         (f32)((texel_ptr_b >> 8) & 0xFF),
                                         (f32)((texel_ptr_b >> 0) & 0xFF),
                                         (f32)((texel_ptr_b >> 24) & 0xFF));
                
                Vec4 texel_c = make_vec4((f32)((texel_ptr_c >> 16) & 0xFF),
                                         (f32)((texel_ptr_c >> 8) & 0xFF),
                                         (f32)((texel_ptr_c >> 0) & 0xFF),
                                         (f32)((texel_ptr_c >> 24) & 0xFF));
                
                Vec4 texel_d = make_vec4((f32)((texel_ptr_d >> 16) & 0xFF),
                                         (f32)((texel_ptr_d >> 8) & 0xFF),
                                         (f32)((texel_ptr_d >> 0) & 0xFF),
                                         (f32)((texel_ptr_d >> 24) & 0xFF));

                // NOTE(xkazu0x): Go from sRGB to "linear" brightness space
                texel_a = srgb255_to_linear1(texel_a);
                texel_b = srgb255_to_linear1(texel_b);
                texel_c = srgb255_to_linear1(texel_c);
                texel_d = srgb255_to_linear1(texel_d);
                
#if 1
                Vec4 texel = lerp(lerp(texel_a, f32_texture_rel_x, texel_b),
                                  f32_texture_rel_y,
                                  lerp(texel_c, f32_texture_rel_x, texel_d));
#else
                Vec4 texel = texel_a;
#endif
                
                Vec4 dest = make_vec4((f32)((*pixel >> 16) & 0xFF),
                                      (f32)((*pixel >> 8) & 0xFF),
                                      (f32)((*pixel >> 0) & 0xFF),
                                      (f32)((*pixel >> 24) & 0xFF));
                
                // NOTE(xkazu0x): Go from sRGB to "linear" brightness space
                dest = srgb255_to_linear1(dest);
                
                f32 rsa = texel.a*color.a;
                f32 rda = dest.a;
                f32 inv_rsa = (1.0f - rsa);
                
                Vec4 blended = make_vec4(inv_rsa*dest.r + color.a*color.r*texel.r,
                                         inv_rsa*dest.g + color.a*color.g*texel.g,
                                         inv_rsa*dest.b + color.a*color.b*texel.b,
                                         (rsa + rda - rsa*rda));
                
                // NOTE(xkazu0x): Go from "linear" brightness space to sRGB
                Vec4 blended255 = linear1_to_srgb255(blended);
                
                *pixel = (((u32)(blended255.a + 0.5f) << 24) |
                          ((u32)(blended255.r + 0.5f) << 16) |
                          ((u32)(blended255.g + 0.5f) << 8) |
                          ((u32)(blended255.b + 0.5f) << 0));
            }
#else
            *pixel = out_color;
#endif
            ++pixel;
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
                
            case RenderEntryType_Render_Entry_Coordinate_System: {
                Render_Entry_Coordinate_System *entry = (Render_Entry_Coordinate_System *)header;
                {
                    draw_rect_slowly(output_target,
                                     entry->origin,
                                     entry->axis_x,
                                     entry->axis_y,
                                     entry->color,
                                     entry->texture);
                    
                    Vec3 point_color = make_vec3(1.0f, 0.0f, 0.0f);
                        
                    Vec2 dim = make_vec2(1.0f);
                    Vec2 point = entry->origin;
                    draw_rect(output_target, point - dim, point + dim, point_color);
                    
                    point = entry->origin + entry->axis_x;
                    draw_rect(output_target, point - dim, point + dim, point_color);
                    
                    point = entry->origin + entry->axis_y;
                    draw_rect(output_target, point - dim, point + dim, point_color);

                    Vec2 max = entry->origin + entry->axis_x + entry->axis_y;
                    draw_rect(output_target, max - dim, max + dim, point_color);
                    
#if 0
                    for (u32 point_index = 0;
                         point_index < ArrayCount(entry->points);
                         ++point_index) {
                        Vec2 p = entry->points[point_index];
                        p = entry->origin + p.x*entry->axis_x + p.y*entry->axis_y;
                        draw_rect(output_target, p - dim, p + dim, entry->color.rgb);
                    }
#endif
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

internal Render_Entry_Coordinate_System *
render_coordinate_system(Render_Group *group, Vec2 origin, Vec2 axis_x, Vec2 axis_y,
                         Vec4 color, Bitmap *texture) {
    Render_Entry_Coordinate_System *entry = render_push(group, Render_Entry_Coordinate_System);
    if (entry) {
        entry->origin = origin;
        entry->axis_x = axis_x;
        entry->axis_y = axis_y;
        entry->color = color;
        entry->texture = texture;
    }
    return(entry);
}
