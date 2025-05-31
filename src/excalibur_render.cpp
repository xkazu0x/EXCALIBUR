internal inline Vec4
unpack4x8(u32 packed) {
    Vec4 result = make_vec4((f32)((packed >> 16) & 0xFF),
                            (f32)((packed >> 8) & 0xFF),
                            (f32)((packed >> 0) & 0xFF),
                            (f32)((packed >> 24) & 0xFF));
    return(result);
}

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

internal Vec4
unscale_and_bias_normal(Vec4 normal) {
    Vec4 result;
    {
        f32 inv255 = 1.0f/255.0f;
        result.x = -1.0f + 2.0f*(inv255*normal.x);
        result.y = -1.0f + 2.0f*(inv255*normal.y);
        result.z = -1.0f + 2.0f*(inv255*normal.z);
        result.w = inv255*normal.w;
    }    
    return(result);
}

struct Bilinear_Sample {
    u32 a;
    u32 b;
    u32 c;
    u32 d;
};

internal inline Bilinear_Sample
bilinear_sample(Bitmap *texture, s32 x, s32 y) {
    Bilinear_Sample result;
    
    u8 *texel_ptr = ((u8 *)texture->memory) + y*texture->pitch + x*BYTES_PER_PIXEL;
    result.a = *(u32 *)(texel_ptr);
    result.b = *(u32 *)(texel_ptr + sizeof(u32));
    result.c = *(u32 *)(texel_ptr + texture->pitch);
    result.d = *(u32 *)(texel_ptr + texture->pitch + sizeof(u32));

    return(result);
}

internal inline Vec4
srgb_bilinear_blend(Bilinear_Sample texel_sample, f32 fx, f32 fy) {
    Vec4 texel_a = unpack4x8(texel_sample.a);
    Vec4 texel_b = unpack4x8(texel_sample.b);
    Vec4 texel_c = unpack4x8(texel_sample.c);
    Vec4 texel_d = unpack4x8(texel_sample.d);

    // NOTE(xkazu0x): Go from sRGB to "linear" brightness space
    texel_a = srgb255_to_linear1(texel_a);
    texel_b = srgb255_to_linear1(texel_b);
    texel_c = srgb255_to_linear1(texel_c);
    texel_d = srgb255_to_linear1(texel_d);
                
    Vec4 result = lerp(lerp(texel_a, fx, texel_b),
                       fy,
                       lerp(texel_c, fx, texel_d));
    return(result);
}

internal inline Vec3
sample_environment_map(Vec2 screen_space, Vec3 sample_direction, f32 roughness, Environment_Map *map,
                       f32 distance_from_map_z) {
    //
    // NOTE(xkazu0x):
    // 1. screen_space tells us where the ray is being cast
    // _from_ in normalized screen coodinates.
    //
    // 2. sample_direction tells us what direction the cast is going
    // - it doest not have to be normalized.
    //
    // 3 .roughness says which LODs of map we sample from.
    //
    // 4. distance_from_map_z says how far the map if from the sample point in Z,
    // given in meters.
    //
    
    // NOTE(xkazu0x): Pick which LOD to sample from.
    u32 lod_index = (u32)(roughness*(f32)(array_count(map->lod) - 1) + 0.5f);
    assert(lod_index < array_count(map->lod));

    Bitmap *lod = map->lod + lod_index;

    // NOTE(xkazu0x): Compute distance to the map
    f32 uvs_per_meter = 0.1f; // TODO(xkazu0x): This needs to be a parameter.
    f32 coefficient = (uvs_per_meter*distance_from_map_z) / sample_direction.y;
    Vec2 offset = coefficient*make_vec2(sample_direction.x, sample_direction.z);

    // NOTE(xkazu0x): Find the intersection point
    Vec2 texture_coord = screen_space + offset;

    // NOTE(xkazu0x): Clamp to the valid range
    texture_coord.u = clamp01(texture_coord.u);
    texture_coord.v = clamp01(texture_coord.v);

    // NOTE(xkazu0x): Bilinear sample
    // TODO(xkazu0x): Formalize texture boundaries!!
    f32 tx = ((texture_coord.u*(f32)(lod->width - 2)));
    f32 ty = ((texture_coord.v*(f32)(lod->height - 2)));
    
    s32 x = (s32)tx;
    s32 y = (s32)ty;

    f32 fx = tx - (f32)x;
    f32 fy = ty - (f32)y;

    assert((x >= 0) && (x < lod->width));
    assert((y >= 0) && (y < lod->height));

#if 0
    u8 *texel_ptr = ((u8 *)lod->memory) + y*lod->pitch + x*sizeof(u32);
    *(u32 *)texel_ptr = 0xFFFFFFFF;
#endif
    
    Bilinear_Sample sample = bilinear_sample(lod, x, y);
    Vec3 result = srgb_bilinear_blend(sample, fx, fy).xyz;
    
    return(result);
}

internal void
draw_rect_slowly(Bitmap *buffer, Vec2 origin, Vec2 x_axis, Vec2 y_axis,
                 Vec4 color, Bitmap *texture, Bitmap *normal_map,
                 Environment_Map *top, Environment_Map *middle,  Environment_Map *bottom,
                 f32 pixels_to_meters) {
    // NOTE(xkazu0x): premultiply color up front
    color.rgb *= color.a;
    
    f32 x_axis_length = length(x_axis);
    f32 y_axis_length = length(y_axis);

    Vec2 normal_x_axis = (y_axis_length / x_axis_length)*x_axis;
    Vec2 normal_y_axis = (x_axis_length / y_axis_length)*y_axis;
    // NOTE(xkazu0x): normal_z_scale could be a parameter if we want people to
    // have control over the amount of scaling in the Z direction
    // that the normals appear to have.
    f32 normal_z_scale = 0.5f*(x_axis_length + y_axis_length);
    
    f32 inv_x_axis_length_squared = 1.0f/length_squared(x_axis);
    f32 inv_y_axis_length_squared = 1.0f/length_squared(y_axis);

    Vec2 points[4] = {
        origin,
        origin + x_axis,
        origin + x_axis + y_axis,
        origin + y_axis,
    };

    s32 width_max = (buffer->width - 1);
    s32 height_max = (buffer->height - 1);

    f32 inv_width_max = 1.0f / (f32)width_max;
    f32 inv_height_max = 1.0f / (f32)height_max;

    // TODO(xkazu0x): This will need to be specified separately!!
    f32 origin_z = 0.0f;
    f32 origin_y = (origin + 0.5f*x_axis + 0.5f*y_axis).y;
    f32 fixed_cast_y = inv_height_max*origin_y;
    
    s32 min_x = width_max;
    s32 max_x = 0;
    
    s32 min_y = height_max;
    s32 max_y = 0;

    for (s32 point_index = 0;
         point_index < array_count(points);
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
            Vec2 pixel_point = make_vec2((f32)x, (f32)y);
            Vec2 pixel_delta = pixel_point - origin;
            
            // TODO(xkazu0x): perp_dot()
            // TODO(xkazu0x): Simpler origin
            f32 edge0 = dot_product(pixel_delta, -perp(x_axis));
            f32 edge1 = dot_product(pixel_delta - x_axis, -perp(y_axis));
            f32 edge2 = dot_product(pixel_delta - x_axis - y_axis, perp(x_axis));
            f32 edge3 = dot_product(pixel_delta - y_axis, perp(y_axis));
           
            if ((edge0 < 0) &&
                (edge1 < 0) &&
                (edge2 < 0) &&
                (edge3 < 0)) {
                Vec2 screen_space = make_vec2(inv_width_max*(f32)x, fixed_cast_y);

                f32 z_diff = pixels_to_meters*((f32)y - origin_y);
                
                f32 u = inv_x_axis_length_squared*dot_product(pixel_delta, x_axis);
                f32 v = inv_y_axis_length_squared*dot_product(pixel_delta, y_axis);

                // TODO(xkazu0x):  SSE clamping
#if 0
                assert((u >= 0.0f) && (u <= 1.0f));
                assert((v >= 0.0f) && (v <= 1.0f));
#endif

                // TODO(xkazu0x): formalize texture boundaries
                f32 tx = ((u*(f32)(texture->width - 2)));
                f32 ty = ((v*(f32)(texture->height - 2)));
                
                s32 texture_x = (s32)tx;
                s32 texture_y = (s32)ty;

                f32 fx = tx - (f32)texture_x;
                f32 fy = ty - (f32)texture_y;

                assert((texture_x >= 0) && (texture_x < texture->width));
                assert((texture_y >= 0) && (texture_y < texture->height));

                Bilinear_Sample texel_sample = bilinear_sample(texture, texture_x, texture_y);
                Vec4 texel = srgb_bilinear_blend(texel_sample, fx, fy);
                
                if (normal_map) {
                    Bilinear_Sample normal_sample = bilinear_sample(normal_map, texture_x, texture_y);
                    
                    Vec4 normal_a = unpack4x8(normal_sample.a);
                    Vec4 normal_b = unpack4x8(normal_sample.b);
                    Vec4 normal_c = unpack4x8(normal_sample.c);
                    Vec4 normal_d = unpack4x8(normal_sample.d);
                
                    Vec4 normal = lerp(lerp(normal_a, fx, normal_b),
                                       fy,
                                       lerp(normal_c, fx, normal_d));
                    normal = unscale_and_bias_normal(normal);
                    
                    normal.xy = normal.x*normal_x_axis + normal.y*normal_y_axis;
                    normal.z *= normal_z_scale;
                    normal.xyz = normalize(normal.xyz);

                    // NOTE(xkazu0x): The eye vector is always assumed to be [0, 0, 1]
                    // This is just the simplified version of the reflection -e + 2e^T N N 
                    Vec3 bounce_direction = 2.0f*normal.z*normal.xyz;
                    bounce_direction.z -= 1.0f;

                    // TODO(xkazu0x): Eventually we need to support two mappings,
                    // one for top-down view (which we don't do now) and one
                    // for sideways, which is what's happening here.
                    bounce_direction.z = -bounce_direction.z;
                    
#if 1
                    Environment_Map *far_map = 0;
                    f32 pos_z = origin_z + z_diff;
                    //f32 map_z = 2.0f;
                    f32 t_env_map = bounce_direction.y;
                    f32 t_far_map = 0.0f;
                    if (t_env_map < -0.5f) {
                        // TODO(xkazu0x): This path seems PARTICALARLY broken.
                        far_map = bottom;
                        t_far_map = -1.0f - 2.0f*t_env_map;
                    } else if (t_env_map > 0.5f) {
                        far_map = top;
                        t_far_map = 2.0f*(t_env_map - 0.5f);
                    }

                    t_far_map *= t_far_map;
                    t_far_map *= t_far_map;
                    
                    Vec3 light_color = make_vec3(0.0f); // TODO(xkazu0x): How do we sample from the middle map?
                    if (far_map) {
                        f32 distance_from_map_z = far_map->pos_z - pos_z;
                        Vec3 far_map_color = sample_environment_map(screen_space, bounce_direction, normal.w, far_map, distance_from_map_z);
                        light_color = lerp(light_color, t_far_map, far_map_color);
                    }
                    
                    texel.rgb = texel.rgb + texel.a*light_color;

#if 0
                    // NOTE(xkazu0x): Draws the bounce direction
                    texel.rgb = make_vec3(0.5f) + 0.5f*bounce_direction;
                    texel.rgb *= texel.a;
#endif
#else                    
                    //texel.rgb = make_vec3(0.5f) + 0.5f*bounce_direction;
                    //texel.r = 0.0f;
                    //texel.g = 0.0f;
                    //texel.b = 0.0f;
                    //texel.a = 1.0f;

                    f32 iso_line = -0.5f;
                    
                    if ((bounce_direction.y >= (iso_line-0.1f)) &&
                        (bounce_direction.y <= (iso_line+0.1f))) {
                        texel.rgb = make_vec3(1.0f);
                    } else {
                        texel.rgb = make_vec3(0.0f);
                    }
                    
                    //texel.rgb = make_vec3(0.5f) + 0.5f*normal.rgb;
                    //texel.a = 1.0f;
#endif
                }
                
                texel = hadamard(texel, color);
                texel.r = clamp01(texel.r);
                texel.g = clamp01(texel.g);
                texel.b = clamp01(texel.b);
                
                Vec4 dest = make_vec4((f32)((*pixel >> 16) & 0xFF),
                                      (f32)((*pixel >> 8) & 0xFF),
                                      (f32)((*pixel >> 0) & 0xFF),
                                      (f32)((*pixel >> 24) & 0xFF));
                
                // NOTE(xkazu0x): Go from sRGB to "linear" brightness space
                dest = srgb255_to_linear1(dest);
                
                Vec4 blended = (1.0f - texel.a)*dest + texel;
                // NOTE(xkazu0x): Go from "linear" brightness space to sRGB
                Vec4 blended255 = linear1_to_srgb255(blended);
                
                *pixel = (((u32)(blended255.a + 0.5f) << 24) |
                          ((u32)(blended255.r + 0.5f) << 16) |
                          ((u32)(blended255.g + 0.5f) << 8) |
                          ((u32)(blended255.b + 0.5f) << 0));
            }
            ++pixel;
        }
        row += buffer->pitch;
    }
}

internal void
draw_rect(Bitmap *buffer, Vec2 min, Vec2 max, Vec4 color) {
    s32 min_x = round_f32_to_s32(min.x);
    s32 min_y = round_f32_to_s32(min.y);
    
    s32 max_x = round_f32_to_s32(max.x);
    s32 max_y = round_f32_to_s32(max.y);

    if (min_x < 0) min_x = 0;
    if (min_y < 0) min_y = 0;
    
    if (max_x > buffer->width) max_x = buffer->width;
    if (max_y > buffer->height) max_y = buffer->height;

    u32 color32 = ((round_f32_to_u32(color.a*255.0f) << 24) |
                   (round_f32_to_u32(color.r*255.0f) << 16) |
                   (round_f32_to_u32(color.g*255.0f) << 8) |
                   (round_f32_to_u32(color.b*255.0f) << 0));
    
    u8 *row = ((u8 *)buffer->memory +
               (min_x*BYTES_PER_PIXEL) +
               (min_y*buffer->pitch));
    for (s32 y = min_y;
         y < max_y;
         ++y) {
        u32 *pixel = (u32 *)row;
        for (s32 x = min_x;
             x < max_x;
             ++x) {
            *pixel++ = color32;
        }
        row += buffer->pitch;
    }
}

internal void
draw_rect_outline(Bitmap *buffer, Vec2 min, Vec2 max, Vec4 color, f32 r) {
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
    for (s32 y = min_y;
         y < max_y;
         ++y) {
        u32 *dest   = (u32 *)dest_row;
        u32 *source = (u32 *)source_row;
        for (s32 x = min_x;
             x < max_x;
             ++x) {
            Vec4 texel = make_vec4((f32)((*source >> 16) & 0xFF),
                                   (f32)((*source >> 8) & 0xFF),
                                   (f32)((*source >> 0) & 0xFF),
                                   (f32)((*source >> 24) & 0xFF));
            texel = srgb255_to_linear1(texel);
            texel *= c_alpha;
            
            Vec4 d = make_vec4((f32)((*dest >> 16) & 0xFF),
                               (f32)((*dest >> 8) & 0xFF),
                               (f32)((*dest >> 0) & 0xFF),
                               (f32)((*dest >> 24) & 0xFF));
            d = srgb255_to_linear1(d);

            Vec4 result = (1.0f - texel.a)*d + texel;
            result = linear1_to_srgb255(result);
            
            *dest = (((u32)(result.a + 0.5f) << 24) |
                     ((u32)(result.r + 0.5f) << 16) |
                     ((u32)(result.g + 0.5f) << 8) |
                     ((u32)(result.b + 0.5f) << 0));
            
            ++dest;
            ++source;
        }
        
        dest_row += buffer->pitch;
        source_row += bitmap->pitch;
    }
}

internal Render_Group *
render_group_alloc(Arena *arena, u32 max_push_buffer_size, u32 resolution_pixels_x, u32 resolution_pixels_y) {
    Render_Group *result = push_struct(arena, Render_Group);
    result->push_buffer_base = (u8 *)push_size(arena, max_push_buffer_size);
    
    result->default_basis = push_struct(arena, Render_Basis);
    result->default_basis->pos = make_vec3(0.0f);
    
    result->max_push_buffer_size = max_push_buffer_size;
    result->push_buffer_size = 0;

    result->game_camera.focal_length = 0.6f; // NOTE(xkazu0x): Meters the person is sitting from the monitor
    result->game_camera.distance_above_target = 9.0f;
    result->game_camera.near_clip_plane = 0.2f;

    result->render_camera = result->game_camera;
    result->render_camera.distance_above_target = 30.0f;
    
    result->global_alpha = 1.0f;

    // TODO(xkazu0x): Need to adjust this based on the buffer size
    f32 width_of_monitor = 0.635f; // NOTE(xkazu0x): Horizontal measurement of monitor in meters
    result->meters_to_pixels = (f32)resolution_pixels_x*width_of_monitor;

    f32 pixels_to_meters = 1.0f / result->meters_to_pixels;
    result->monitor_half_dim_in_meters = make_vec2(0.5f*resolution_pixels_x*pixels_to_meters,
                                                   0.5f*resolution_pixels_y*pixels_to_meters);
    
    return(result);
}

struct Entity_Basis_Point {
    Vec2 point;
    f32 scale;
    b32 valid;
};

internal Entity_Basis_Point
get_render_entity_basis_point(Render_Group *group, Render_Entity_Basis *entity_basis, Vec2 screen_dim) {
    Entity_Basis_Point result = {};

    Vec2 screen_center = 0.5f*screen_dim;
    
    Vec3 entity_base_point = entity_basis->basis->pos;
    
    f32 distance_to_point = group->render_camera.distance_above_target - entity_base_point.z;
    
    Vec3 raw_point = make_vec3(entity_base_point.xy + entity_basis->offset.xy, 1.0f);
    
    if (distance_to_point > group->render_camera.near_clip_plane) {
        Vec3 projected_point = (1.0f / distance_to_point)*group->render_camera.focal_length*raw_point;
        result.point = screen_center + group->meters_to_pixels*projected_point.xy;
        result.scale = group->meters_to_pixels*projected_point.z;
        result.valid = true;
    }
    
    return(result);
}

internal void
render_group_draw(Render_Group *group, Bitmap *output_target) {
    Vec2 screen_dim = make_vec2((f32)output_target->width, (f32)output_target->height);
    
    f32 pixels_to_meters = 1.0f / group->meters_to_pixels;

    for (u32 base_address = 0;
         base_address < group->push_buffer_size;
         ) {
        Render_Entry_Header *header = (Render_Entry_Header *)(group->push_buffer_base + base_address);
        base_address += sizeof(*header);
        void *data = (u8 *)header + sizeof(*header);
        
        switch (header->type) {
            case RenderEntryType_Render_Entry_Clear: {
                Render_Entry_Clear *entry = (Render_Entry_Clear *)data;
                {
                    draw_rect(output_target,
                              make_vec2(0.0f),
                              make_vec2((f32)output_target->width, (f32)output_target->height),
                              entry->color);
                }
                base_address += sizeof(*entry);
            } break;
                
            case RenderEntryType_Render_Entry_Rect: {
                Render_Entry_Rect *entry = (Render_Entry_Rect *)data;
                {
                    Entity_Basis_Point basis = get_render_entity_basis_point(group, &entry->entity_basis, screen_dim);
                    draw_rect(output_target, basis.point, basis.point + basis.scale*entry->dim, entry->color);
                }
                base_address += sizeof(*entry);
            } break;
                
            case RenderEntryType_Render_Entry_Bitmap: {
                Render_Entry_Bitmap *entry = (Render_Entry_Bitmap *)data;
                {
                    assert(entry->bitmap);
                    Entity_Basis_Point basis = get_render_entity_basis_point(group, &entry->entity_basis, screen_dim);
#if 0
                    draw_bitmap(output_target, entry->bitmap, basis.point, entry->color.a);
#else
                    draw_rect_slowly(output_target, basis.point,
                                     basis.scale*make_vec2(entry->size.x, 0.0f),
                                     basis.scale*make_vec2(0.0f, entry->size.y),
                                     entry->color, entry->bitmap, 0, 0, 0, 0, pixels_to_meters);
#endif
                }
                base_address += sizeof(*entry);
            } break;
                
            case RenderEntryType_Render_Entry_Coordinate_System: {
                Render_Entry_Coordinate_System *entry = (Render_Entry_Coordinate_System *)data;
                {
                    draw_rect_slowly(output_target,
                                     entry->origin,
                                     entry->x_axis,
                                     entry->y_axis,
                                     entry->color,
                                     entry->texture,
                                     entry->normal_map,
                                     entry->top,
                                     entry->middle,
                                     entry->bottom,
                                     pixels_to_meters);
                    
                    Vec4 point_color = make_vec4(1.0f, 0.5f, 0.3f, 1.0f);
                        
                    Vec2 dim = make_vec2(1.0f);
                    Vec2 point = entry->origin;
                    draw_rect(output_target, point - dim, point + dim, point_color);
                    
                    point = entry->origin + entry->x_axis;
                    draw_rect(output_target, point - dim, point + dim, point_color);
                    
                    point = entry->origin + entry->y_axis;
                    draw_rect(output_target, point - dim, point + dim, point_color);

                    Vec2 max = entry->origin + entry->x_axis + entry->y_axis;
                    draw_rect(output_target, max - dim, max + dim, point_color);
                    
#if 0
                    for (u32 point_index = 0;
                         point_index < array_count(entry->points);
                         ++point_index) {
                        Vec2 p = entry->points[point_index];
                        p = entry->origin + p.x*entry->x_axis + p.y*entry->y_axis;
                        draw_rect(output_target, p - dim, p + dim, entry->color.rgb);
                    }
#endif
                }
                base_address += sizeof(*entry);
            } break;
            {
                invalid_default_case;
            }
        }
    }
}

internal void *
render_push_(Render_Group *group, u32 size, Render_Entry_Type type) {
    void *result = 0;
    size += sizeof(Render_Entry_Header);
    if ((group->push_buffer_size + size) < group->max_push_buffer_size) {
        Render_Entry_Header *header = (Render_Entry_Header *)(group->push_buffer_base + group->push_buffer_size);
        header->type = type;
        result = (u8 *)header + sizeof(*header);
        group->push_buffer_size += size;
    } else {
        invalid_path;
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
render_rect(Render_Group *group, Vec3 offset, Vec2 dim, Vec4 color) {
    Render_Entry_Rect *entry = render_push(group, Render_Entry_Rect);
    if (entry) {
        entry->entity_basis.basis = group->default_basis;
        entry->entity_basis.offset = offset - make_vec3(0.5f*dim, 0.0f);
        entry->dim = dim;
        entry->color = color;
    }
}

internal void
render_rect_outline(Render_Group *group, Vec3 offset, Vec2 dim, Vec4 color) {
    f32 thickness = 0.2f;
    
    // NOTE(xkazu0x): top and bottom
    render_rect(group, offset - make_vec3(0.0f, 0.5f*dim.y, 0.0f), make_vec2(dim.x, thickness), color);
    render_rect(group, offset + make_vec3(0.0f, 0.5f*dim.y, 0.0f), make_vec2(dim.x, thickness), color);
    
    // NOTE(xkazu0x): left and right
    render_rect(group, offset - make_vec3(0.5f*dim.x, 0.0f, 0.0f), make_vec2(thickness, dim.y), color);
    render_rect(group, offset + make_vec3(0.5f*dim.x, 0.0f, 0.0f), make_vec2(thickness, dim.y), color);
}

internal void
render_bitmap(Render_Group *group, Bitmap *bitmap, Vec3 offset, f32 height, Vec4 color) {
    Render_Entry_Bitmap *entry = render_push(group, Render_Entry_Bitmap);
    if (entry) {
        entry->entity_basis.basis = group->default_basis;

        Vec2 size = make_vec2(height*bitmap->width_over_height, height);
        Vec2 pixel_align = hadamard(bitmap->align_percentage, size);
        
        entry->entity_basis.offset = offset - make_vec3(pixel_align, 0.0f);
        
        entry->bitmap = bitmap;
        entry->size = size;
        entry->color = group->global_alpha*color;
    }
}

// NOTE(xkazu0x): This is just for test
// {
internal void
render_coordinate_system(Render_Group *group,
                         Vec2 origin, Vec2 x_axis, Vec2 y_axis,
                         Vec4 color, Bitmap *texture, Bitmap *normal_map,
                         Environment_Map *top, Environment_Map *middle, Environment_Map *bottom) {
    Render_Entry_Coordinate_System *entry = render_push(group, Render_Entry_Coordinate_System);
    if (entry) {
        entry->origin = origin;
        entry->x_axis = x_axis;
        entry->y_axis = y_axis;
        entry->color = color;
        entry->texture = texture;
        entry->normal_map = normal_map;
        entry->top = top;
        entry->middle = middle;
        entry->bottom = bottom;
    }
}
// }

internal Vec2
unproject(Render_Group *group, Vec2 projected_point, f32 at_distance_from_camera) {
    Vec2 result = (at_distance_from_camera / group->game_camera.focal_length)*projected_point;
    return(result);
}

internal Rect2
get_camera_rect_at_distance(Render_Group *group, f32 distance_from_camera) {
    Vec2 raw_point = unproject(group, group->monitor_half_dim_in_meters, distance_from_camera);
    Rect2 result = make_rect2_center_half_dim(make_vec2(0.0f), raw_point);
    return(result);
}

internal Rect2
get_camera_rect_at_target(Render_Group *group) {
    Rect2 result = get_camera_rect_at_distance(group, group->game_camera.distance_above_target);
    return(result);
}
