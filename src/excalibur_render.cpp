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
    
    u8 *texel_ptr = ((u8 *)texture->memory) + y*texture->pitch + x*BITMAP_BYTES_PER_PIXEL;
    result.a = *(u32 *)(texel_ptr);
    result.b = *(u32 *)(texel_ptr + BITMAP_BYTES_PER_PIXEL);
    result.c = *(u32 *)(texel_ptr + texture->pitch);
    result.d = *(u32 *)(texel_ptr + texture->pitch + BITMAP_BYTES_PER_PIXEL);

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
    BEGIN_TIMED_BLOCK(draw_rect_slowly);
    
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
                
                texel = hadamard_product(texel, color);
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

    END_TIMED_BLOCK(draw_rect_slowly);
}

internal void
draw_rect_quickly(Bitmap *buffer, Vec2 origin, Vec2 x_axis, Vec2 y_axis, Vec4 color, Bitmap *texture, f32 pixels_to_meters, Rect2i clip_rect, b32 even) {
    BEGIN_TIMED_BLOCK(draw_rect_quickly);    
    
    color.rgb *= color.a;
    
    f32 x_axis_length = length(x_axis);
    f32 y_axis_length = length(y_axis);

    Vec2 normal_x_axis = (y_axis_length/x_axis_length)*x_axis;
    Vec2 normal_y_axis = (x_axis_length/y_axis_length)*y_axis;
    
    f32 inv_x_axis_length_squared = 1.0f/length_squared(x_axis);
    f32 inv_y_axis_length_squared = 1.0f/length_squared(y_axis);
    
    Rect2i fill_rect = rect2i_inverted_infinity();
    
    Vec2 points[4] = {
        origin,
        origin + x_axis,
        origin + x_axis + y_axis,
        origin + y_axis,
    };

    for (s32 point_index = 0;
         point_index < array_count(points);
         ++point_index) {
        Vec2 test_point = points[point_index];
        
        s32 floor_x = (s32)floor_f32(test_point.x);
        s32 floor_y = (s32)floor_f32(test_point.y);
        s32 ceil_x =  (s32)ceil_f32(test_point.x) + 1;
        s32 ceil_y =  (s32)ceil_f32(test_point.y) + 1;

        if (fill_rect.min_x > floor_x) fill_rect.min_x = floor_x;
        if (fill_rect.min_y > floor_y) fill_rect.min_y = floor_y;
        if (fill_rect.max_x < ceil_x)  fill_rect.max_x = ceil_x;
        if (fill_rect.max_y < ceil_y)  fill_rect.max_y = ceil_y;
    }

    //Rect2i clip_rect = make_rect2i(0, 1, width_max, height_max);
    //Rect2i clip_rect = make_rect2i(128, 256);
    fill_rect = rect2i_intersect(fill_rect, clip_rect);
    
    if (!even == (fill_rect.min_y & 1)) fill_rect.min_y += 1;

    if (has_rect2i_area(fill_rect)) {
        __m128i start_clip_mask = _mm_set1_epi8(-1);
        __m128i end_clip_mask = _mm_set1_epi8(-1);

        __m128i start_clip_masks[] = {
            _mm_slli_si128(start_clip_mask, 0*4),
            _mm_slli_si128(start_clip_mask, 1*4),
            _mm_slli_si128(start_clip_mask, 2*4),
            _mm_slli_si128(start_clip_mask, 3*4),
        };
        
        __m128i end_clip_masks[] = {
            _mm_srli_si128(end_clip_mask, 0*4),
            _mm_srli_si128(end_clip_mask, 3*4),
            _mm_srli_si128(end_clip_mask, 2*4),
            _mm_srli_si128(end_clip_mask, 1*4),
        };
        
        if (fill_rect.min_x & 3) {
            start_clip_mask = start_clip_masks[fill_rect.min_x & 3];
            fill_rect.min_x = fill_rect.min_x & ~3;
        }

        if (fill_rect.max_x & 3) {
            end_clip_mask = end_clip_masks[fill_rect.max_x & 3];
            fill_rect.max_x = (fill_rect.max_x & ~3) + 4;
        }
            
        Vec2 n_x_axis = inv_x_axis_length_squared*x_axis;
        Vec2 n_y_axis = inv_y_axis_length_squared*y_axis;

        __m128 n_x_axis_x_4x = _mm_set1_ps(n_x_axis.x);
        __m128 n_x_axis_y_4x = _mm_set1_ps(n_x_axis.y);
            
        __m128 n_y_axis_x_4x = _mm_set1_ps(n_y_axis.x);
        __m128 n_y_axis_y_4x = _mm_set1_ps(n_y_axis.y);
    
        __m128 inv255_4x = _mm_set1_ps(1.0f/255.0f);
        __m128 one255_4x = _mm_set1_ps(255.0f);

        __m128i mask_ff = _mm_set1_epi32(0xFF);
        __m128i mask_ffff = _mm_set1_epi32(0xFFFF);
        __m128i mask_ff00ff = _mm_set1_epi32(0xFF00FF);

        __m128 max_color_value = _mm_set1_ps(square(255.0f));
    
        __m128 color_r_4x = _mm_set1_ps(color.r);
        __m128 color_g_4x = _mm_set1_ps(color.g);
        __m128 color_b_4x = _mm_set1_ps(color.b);
        __m128 color_a_4x = _mm_set1_ps(color.a);
    
        __m128 one_4x = _mm_set1_ps(1.0f);
        __m128 half_4x = _mm_set1_ps(0.5f);
        __m128 zero_4x = _mm_set1_ps(0.0f);
        __m128 four_4x = _mm_set1_ps(4.0f);
    
        __m128 origin_x_4x = _mm_set1_ps(origin.x);
        __m128 origin_y_4x = _mm_set1_ps(origin.y);

        __m128 width_m2 = _mm_set1_ps((f32)(texture->width - 2));
        __m128 height_m2 = _mm_set1_ps((f32)(texture->height - 2));
    
        __m128i texture_pitch_4x = _mm_set1_epi32(texture->pitch);
    
        s32 texture_pitch = texture->pitch;
        void *texture_memory = texture->memory;

        s32 min_x = fill_rect.min_x;
        s32 min_y = fill_rect.min_y;
        s32 max_x = fill_rect.max_x;
        s32 max_y = fill_rect.max_y;
    
        u8 *row = ((u8 *)buffer->memory +
                   (min_x*BITMAP_BYTES_PER_PIXEL) +
                   (min_y*buffer->pitch));
    
        u32 row_advance = 2*buffer->pitch;
    
        BEGIN_TIMED_BLOCK(process_pixel);    
        for (s32 y = min_y;
             y < max_y;
             y += 2) {
            __m128 pixel_y = _mm_sub_ps(_mm_set1_ps((f32)y), origin_y_4x);
            __m128 pixel_x = _mm_sub_ps(_mm_set_ps((f32)(min_x + 3),
                                                   (f32)(min_x + 2),
                                                   (f32)(min_x + 1),
                                                   (f32)(min_x + 0)),
                                        origin_x_4x);

            __m128i clip_mask = start_clip_mask;
        
            u32 *pixel = (u32 *)row;
            for (s32 x = min_x;
                 x < max_x;
                 x += 4) {
#define mf(x, index) ((f32 *)&(x))[index]
#define mi(x, index) ((u32 *)&(x))[index]
#define mm_square(x) _mm_mul_ps(x, x)
            
                __m128 u = _mm_add_ps(_mm_mul_ps(pixel_x, n_x_axis_x_4x), _mm_mul_ps(pixel_y, n_x_axis_y_4x));
                __m128 v = _mm_add_ps(_mm_mul_ps(pixel_x, n_y_axis_x_4x), _mm_mul_ps(pixel_y, n_y_axis_y_4x));
            
                __m128i write_mask = _mm_castps_si128(_mm_and_ps(_mm_and_ps(_mm_cmpge_ps(u, zero_4x),
                                                                            _mm_cmple_ps(u, one_4x)),
                                                                 _mm_and_ps(_mm_cmpge_ps(v, zero_4x),
                                                                            _mm_cmple_ps(v, one_4x))));
                write_mask = _mm_and_si128(write_mask, clip_mask);

                // TODO(xkazu0x): Later, re-check if this helps
                // if (_mm_movemask_epi8(write_mask))
                {
                    __m128i original_dest = _mm_load_si128((__m128i *)pixel);
            
                    u = _mm_min_ps(_mm_max_ps(u, zero_4x), one_4x);
                    v = _mm_min_ps(_mm_max_ps(v, zero_4x), one_4x);

                    // NOTE(xkazu0x): Bias texture coordinates to start on the boundary between the 0,0 and 1,1 pixels.
                    __m128 tx = _mm_add_ps(_mm_mul_ps(u, width_m2), half_4x);
                    __m128 ty = _mm_add_ps(_mm_mul_ps(v, height_m2), half_4x);

                    __m128i fetch_x_4x = _mm_cvttps_epi32(tx);
                    __m128i fetch_y_4x = _mm_cvttps_epi32(ty);

                    __m128 fx = _mm_sub_ps(tx, _mm_cvtepi32_ps(fetch_x_4x));
                    __m128 fy = _mm_sub_ps(ty, _mm_cvtepi32_ps(fetch_y_4x));

                    fetch_x_4x = _mm_slli_epi32(fetch_x_4x, 2);
                    fetch_y_4x = _mm_or_si128(_mm_mullo_epi16(fetch_y_4x, texture_pitch_4x),
                                              _mm_slli_epi32(_mm_mulhi_epi16(fetch_y_4x, texture_pitch_4x), 16));
                    __m128i fetch_4x = _mm_add_epi32(fetch_x_4x, fetch_y_4x);

                    u8 *texel_ptr0 = ((u8 *)texture_memory) + mi(fetch_4x, 0);
                    u8 *texel_ptr1 = ((u8 *)texture_memory) + mi(fetch_4x, 1);
                    u8 *texel_ptr2 = ((u8 *)texture_memory) + mi(fetch_4x, 2);
                    u8 *texel_ptr3 = ((u8 *)texture_memory) + mi(fetch_4x, 3);
                
                    __m128i sample_a = _mm_setr_epi32(*(u32 *)(texel_ptr0),
                                                      *(u32 *)(texel_ptr1),
                                                      *(u32 *)(texel_ptr2),
                                                      *(u32 *)(texel_ptr3));
                
                    __m128i sample_b = _mm_setr_epi32(*(u32 *)(texel_ptr0 + BITMAP_BYTES_PER_PIXEL),
                                                      *(u32 *)(texel_ptr1 + BITMAP_BYTES_PER_PIXEL),
                                                      *(u32 *)(texel_ptr2 + BITMAP_BYTES_PER_PIXEL),
                                                      *(u32 *)(texel_ptr3 + BITMAP_BYTES_PER_PIXEL));

                    __m128i sample_c = _mm_setr_epi32(*(u32 *)(texel_ptr0 + texture_pitch),
                                                      *(u32 *)(texel_ptr1 + texture_pitch),
                                                      *(u32 *)(texel_ptr2 + texture_pitch),
                                                      *(u32 *)(texel_ptr3 + texture_pitch));

                    __m128i sample_d = _mm_setr_epi32(*(u32 *)(texel_ptr0 + texture_pitch + BITMAP_BYTES_PER_PIXEL),
                                                      *(u32 *)(texel_ptr1 + texture_pitch + BITMAP_BYTES_PER_PIXEL),
                                                      *(u32 *)(texel_ptr2 + texture_pitch + BITMAP_BYTES_PER_PIXEL),
                                                      *(u32 *)(texel_ptr3 + texture_pitch + BITMAP_BYTES_PER_PIXEL));
                
#if 1
                    // NOTE(xkazu0x): Unpack bilinear samples
                    __m128i texel_a_rb = _mm_and_si128(sample_a, mask_ff00ff);
                    __m128i texel_a_ag = _mm_and_si128(_mm_srli_epi32(sample_a, 8), mask_ff00ff);
                    texel_a_rb = _mm_mullo_epi16(texel_a_rb, texel_a_rb);
                    __m128 texel_a_a = _mm_cvtepi32_ps(_mm_srli_epi32(texel_a_ag, 16));
                    texel_a_ag = _mm_mullo_epi16(texel_a_ag, texel_a_ag);
                
                    __m128i texel_b_rb = _mm_and_si128(sample_b, mask_ff00ff);
                    __m128i texel_b_ag = _mm_and_si128(_mm_srli_epi32(sample_b, 8), mask_ff00ff);
                    texel_b_rb = _mm_mullo_epi16(texel_b_rb, texel_b_rb);
                    __m128 texel_b_a = _mm_cvtepi32_ps(_mm_srli_epi32(texel_b_ag, 16));
                    texel_b_ag = _mm_mullo_epi16(texel_b_ag, texel_b_ag);

                    __m128i texel_c_rb = _mm_and_si128(sample_c, mask_ff00ff);
                    __m128i texel_c_ag = _mm_and_si128(_mm_srli_epi32(sample_c, 8), mask_ff00ff);
                    texel_c_rb = _mm_mullo_epi16(texel_c_rb, texel_c_rb);
                    __m128 texel_c_a = _mm_cvtepi32_ps(_mm_srli_epi32(texel_c_ag, 16));
                    texel_c_ag = _mm_mullo_epi16(texel_c_ag, texel_c_ag);

                    __m128i texel_d_rb = _mm_and_si128(sample_d, mask_ff00ff);
                    __m128i texel_d_ag = _mm_and_si128(_mm_srli_epi32(sample_d, 8), mask_ff00ff);
                    texel_d_rb = _mm_mullo_epi16(texel_d_rb, texel_d_rb);
                    __m128 texel_d_a = _mm_cvtepi32_ps(_mm_srli_epi32(texel_d_ag, 16));
                    texel_d_ag = _mm_mullo_epi16(texel_d_ag, texel_d_ag);

                    // NOTE(xkazu0x): convert texture from 0-255 sRGB to "linear" 0-1 brightness space
                    __m128 texel_a_r = _mm_cvtepi32_ps(_mm_srli_epi32(texel_a_rb, 16));
                    __m128 texel_a_g = _mm_cvtepi32_ps(_mm_and_si128(texel_a_ag, mask_ffff));
                    __m128 texel_a_b = _mm_cvtepi32_ps(_mm_and_si128(texel_a_rb, mask_ffff));

                    __m128 texel_b_r = _mm_cvtepi32_ps(_mm_srli_epi32(texel_b_rb, 16));
                    __m128 texel_b_g = _mm_cvtepi32_ps(_mm_and_si128(texel_b_ag, mask_ffff));
                    __m128 texel_b_b = _mm_cvtepi32_ps(_mm_and_si128(texel_b_rb, mask_ffff));
                
                    __m128 texel_c_r = _mm_cvtepi32_ps(_mm_srli_epi32(texel_c_rb, 16));
                    __m128 texel_c_g = _mm_cvtepi32_ps(_mm_and_si128(texel_c_ag, mask_ffff));
                    __m128 texel_c_b = _mm_cvtepi32_ps(_mm_and_si128(texel_c_rb, mask_ffff));
                
                    __m128 texel_d_r = _mm_cvtepi32_ps(_mm_srli_epi32(texel_d_rb, 16));
                    __m128 texel_d_g = _mm_cvtepi32_ps(_mm_and_si128(texel_d_ag, mask_ffff));
                    __m128 texel_d_b = _mm_cvtepi32_ps(_mm_and_si128(texel_d_rb, mask_ffff));
#else
                    // NOTE(xkazu0x): Unpack bilinear samples
                    __m128 texel_a_b = _mm_cvtepi32_ps(_mm_and_si128(sample_a, mask_ff));
                    __m128 texel_a_g = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(sample_a, 8), mask_ff));
                    __m128 texel_a_r = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(sample_a, 16), mask_ff));
                    __m128 texel_a_a = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(sample_a, 24), mask_ff));
          
                    __m128 texel_b_b = _mm_cvtepi32_ps(_mm_and_si128(sample_b, mask_ff));
                    __m128 texel_b_g = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(sample_b, 8), mask_ff));
                    __m128 texel_b_r = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(sample_b, 16), mask_ff));
                    __m128 texel_b_a = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(sample_b, 24), mask_ff));
            
                    __m128 texel_c_b = _mm_cvtepi32_ps(_mm_and_si128(sample_c, mask_ff));
                    __m128 texel_c_g = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(sample_c, 8), mask_ff));
                    __m128 texel_c_r = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(sample_c, 16), mask_ff));
                    __m128 texel_c_a = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(sample_c, 24), mask_ff));
            
                    __m128 texel_d_b = _mm_cvtepi32_ps(_mm_and_si128(sample_d, mask_ff));
                    __m128 texel_d_g = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(sample_d, 8), mask_ff));
                    __m128 texel_d_r = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(sample_d, 16), mask_ff));
                    __m128 texel_d_a = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(sample_d, 24), mask_ff));
                
                    // NOTE(xkazu0x): convert texture from 0-255 sRGB to "linear" 0-1 brightness space                
                    texel_a_r = mm_square(texel_a_r);
                    texel_a_g = mm_square(texel_a_g);
                    texel_a_b = mm_square(texel_a_b);
            
                    texel_b_r = mm_square(texel_b_r);
                    texel_b_g = mm_square(texel_b_g);
                    texel_b_b = mm_square(texel_b_b);
            
                    texel_c_r = mm_square(texel_c_r);
                    texel_c_g = mm_square(texel_c_g);
                    texel_c_b = mm_square(texel_c_b);
            
                    texel_d_r = mm_square(texel_d_r);
                    texel_d_g = mm_square(texel_d_g);
                    texel_d_b = mm_square(texel_d_b);
#endif
                
                    // NOTE(xkazu0x): Load destination            
                    __m128 dest_b = _mm_cvtepi32_ps(_mm_and_si128(original_dest, mask_ff));
                    __m128 dest_g = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(original_dest, 8), mask_ff));
                    __m128 dest_r = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(original_dest, 16), mask_ff));
                    __m128 dest_a = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(original_dest, 24), mask_ff));
                
                    // NOTE(xkazu0x): Bilinear texture blend
                    __m128 ifx = _mm_sub_ps(one_4x, fx);
                    __m128 ify = _mm_sub_ps(one_4x, fy);

                    __m128 l0 = _mm_mul_ps(ify, ifx);
                    __m128 l1 = _mm_mul_ps(ify, fx);
                    __m128 l2 = _mm_mul_ps(fy, ifx);
                    __m128 l3 = _mm_mul_ps(fy, fx);

                    __m128 texel_r = _mm_add_ps(_mm_add_ps(_mm_mul_ps(l0, texel_a_r), _mm_mul_ps(l1, texel_b_r)),
                                                _mm_add_ps(_mm_mul_ps(l2, texel_c_r), _mm_mul_ps(l3, texel_d_r)));
                    __m128 texel_g = _mm_add_ps(_mm_add_ps(_mm_mul_ps(l0, texel_a_g), _mm_mul_ps(l1, texel_b_g)),
                                                _mm_add_ps(_mm_mul_ps(l2, texel_c_g), _mm_mul_ps(l3, texel_d_g)));
                    __m128 texel_b = _mm_add_ps(_mm_add_ps(_mm_mul_ps(l0, texel_a_b), _mm_mul_ps(l1, texel_b_b)),
                                                _mm_add_ps(_mm_mul_ps(l2, texel_c_b), _mm_mul_ps(l3, texel_d_b)));
                    __m128 texel_a = _mm_add_ps(_mm_add_ps(_mm_mul_ps(l0, texel_a_a), _mm_mul_ps(l1, texel_b_a)),
                                                _mm_add_ps(_mm_mul_ps(l2, texel_c_a), _mm_mul_ps(l3, texel_d_a)));
            
                    // NOTE(xkazu0x): Module by incoming color
                    texel_r = _mm_mul_ps(texel_r, color_r_4x);
                    texel_g = _mm_mul_ps(texel_g, color_g_4x);
                    texel_b = _mm_mul_ps(texel_b, color_b_4x);
                    texel_a = _mm_mul_ps(texel_a, color_a_4x);
            
                    // NOTE(xkazu0x): Clamp colors to valid range 255.0f*255.0f
                    texel_r = _mm_min_ps(_mm_max_ps(texel_r, zero_4x), max_color_value);
                    texel_g = _mm_min_ps(_mm_max_ps(texel_g, zero_4x), max_color_value);
                    texel_b = _mm_min_ps(_mm_max_ps(texel_b, zero_4x), max_color_value);
                    texel_a = _mm_min_ps(_mm_max_ps(texel_a, zero_4x), max_color_value);
                
                    // NOTE(xkazu0x): Go from sRGB to "linear" brightness space
                    dest_r = mm_square(dest_r);
                    dest_g = mm_square(dest_g);
                    dest_b = mm_square(dest_b);
                
                    // NOTE(xkazu0x): Destination blend
                    __m128 inv_texel_a = _mm_sub_ps(one_4x, _mm_mul_ps(inv255_4x, texel_a));
                    __m128 blended_r = _mm_add_ps(_mm_mul_ps(inv_texel_a, dest_r), texel_r);
                    __m128 blended_g = _mm_add_ps(_mm_mul_ps(inv_texel_a, dest_g), texel_g);
                    __m128 blended_b = _mm_add_ps(_mm_mul_ps(inv_texel_a, dest_b), texel_b);
                    __m128 blended_a = _mm_add_ps(_mm_mul_ps(inv_texel_a, dest_a), texel_a);

                    // TODO(xkazu0x): The prior step may generate a negative number that cause
                    // the square root function to throw an error, so we clamp that value to zero.
                    // for (s32 pixel_index = 0;
                    //      pixel_index < 4;
                    //      ++pixel_index) {
                    //     if (mf(blended_r, pixel_index) < 0.0f) mf(blended_r, pixel_index) = 0.0f;
                    //     if (mf(blended_g, pixel_index) < 0.0f) mf(blended_g, pixel_index) = 0.0f;
                    //     if (mf(blended_b, pixel_index) < 0.0f) mf(blended_b, pixel_index) = 0.0f;
                    //     if (mf(blended_a, pixel_index) < 0.0f) mf(blended_a, pixel_index) = 0.0f;
                    // }
                
                    // NOTE(xkazu0x): Go from "linear" brightness space to sRGB
#if 1
                    blended_r = _mm_mul_ps(blended_r, _mm_rsqrt_ps(blended_r));
                    blended_g = _mm_mul_ps(blended_g, _mm_rsqrt_ps(blended_g));
                    blended_b = _mm_mul_ps(blended_b, _mm_rsqrt_ps(blended_b));
#else
                    blended_r = _mm_sqrt_ps(blended_r);
                    blended_g = _mm_sqrt_ps(blended_g);
                    blended_b = _mm_sqrt_ps(blended_b);
#endif
                    __m128i int_r = _mm_cvtps_epi32(blended_r);
                    __m128i int_g = _mm_cvtps_epi32(blended_g);
                    __m128i int_b = _mm_cvtps_epi32(blended_b);
                    __m128i int_a = _mm_cvtps_epi32(blended_a);

                    __m128i sr = _mm_slli_epi32(int_r, 16);
                    __m128i sg = _mm_slli_epi32(int_g, 8);
                    __m128i sb = int_b;
                    __m128i sa = _mm_slli_epi32(int_a, 24);

                    __m128i out = _mm_or_si128(_mm_or_si128(sr, sg), _mm_or_si128(sb, sa));

                    __m128i masked_out = _mm_or_si128(_mm_and_si128(write_mask, out),
                                                      _mm_andnot_si128(write_mask, original_dest));
                
                    _mm_store_si128((__m128i *)pixel, masked_out);
                }
                pixel_x = _mm_add_ps(pixel_x, four_4x);
                pixel += 4;

                if ((x + 8) < max_x) {
                    clip_mask = _mm_set1_epi8(-1);
                } else {
                    clip_mask = end_clip_mask;
                }
            }
            row += row_advance;
        }
        END_TIMED_BLOCK_COUNTED(process_pixel, get_rect2i_clamped_area(fill_rect)/2);
    }
    
    END_TIMED_BLOCK(draw_rect_quickly);
}

internal void
draw_rect(Bitmap *buffer, Vec2 min, Vec2 max, Vec4 color, Rect2i clip_rect, b32 even) {
    Rect2i fill_rect = make_rect2i((s32)round_f32(min.x),
                                   (s32)round_f32(min.y),
                                   (s32)round_f32(max.x),
                                   (s32)round_f32(max.y));
    fill_rect = rect2i_intersect(fill_rect, clip_rect);
    if (!even == (fill_rect.min_y & 1)) fill_rect.min_y += 1;
    
    u32 color32 = (((u32)round_f32(color.a*255.0f) << 24) |
                   ((u32)round_f32(color.r*255.0f) << 16) |
                   ((u32)round_f32(color.g*255.0f) << 8) |
                   ((u32)round_f32(color.b*255.0f) << 0));
    
    u8 *row = ((u8 *)buffer->memory +
               (fill_rect.min_x*BYTES_PER_PIXEL) +
               (fill_rect.min_y*buffer->pitch));
    
    for (s32 y = fill_rect.min_y;
         y < fill_rect.max_y;
         y += 2) {
        u32 *pixel = (u32 *)row;
        for (s32 x = fill_rect.min_x;
             x < fill_rect.max_x;
             ++x) {
            *pixel++ = color32;
        }
        row += 2*buffer->pitch;
    }
}

internal void
draw_bitmap(Bitmap *buffer, Bitmap *bitmap, Vec2 offset, f32 c_alpha) {
    s32 min_x = (s32)round_f32(offset.x);
    s32 min_y = (s32)round_f32(offset.y);
    
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

struct Project_Point_Result {
    Vec2 point;
    f32 scale;
    b32 valid;
};

internal Project_Point_Result
project_point(Render_Transform *transform, Vec3 point) {
    Project_Point_Result result = {};

    Vec3 new_point = make_vec3(point.xy, 0.0f) + transform->offset;

    if (transform->orthographic) {
        result.point = transform->screen_center + transform->meters_to_pixels*new_point.xy;
        result.scale = transform->meters_to_pixels;
        result.valid = true;
    } else {
        Vec3 raw_point = make_vec3(new_point.xy, 1.0f);
        f32 distance_above_target = transform->distance_above_target;
#if 0
        distance_above_target = 40.0f;
#endif
        f32 distance_to_point_z = distance_above_target - new_point.z;
        
        f32 offset_z = 0.0f;
        
        if (distance_to_point_z > transform->near_clip_plane) {
            Vec3 projected_point = (1.0f / distance_to_point_z)*transform->focal_length*raw_point;
            result.scale = transform->meters_to_pixels*projected_point.z;
            result.point = transform->screen_center + transform->meters_to_pixels*projected_point.xy + make_vec2(0.0f, result.scale*offset_z);
            result.valid = true;
        }
    }
    
    return(result);
}

//
//
//

internal Render_Group *
render_group_alloc(Asset_Manager *asset_manager, Arena *arena, u32 max_push_buffer_size) {
    Render_Group *group = push_struct(arena, Render_Group);
    if (max_push_buffer_size == 0) {
        max_push_buffer_size = (u32)get_arena_size_remaining(arena);
    }
    group->push_buffer_base = (u8 *)push_size(arena, max_push_buffer_size);
    
    group->max_push_buffer_size = max_push_buffer_size;
    group->push_buffer_size = 0;

    group->asset_manager = asset_manager;
    group->global_alpha = 1.0f;

    group->transform.offset = make_vec3(0.0f);
    group->transform.scale = 1.0f;

    group->missing_resource_count = 0;
    
    return(group);
}

internal void
render_group_draw(Render_Group *group, Bitmap *output_target, Rect2i clip_rect, b32 even) {
    BEGIN_TIMED_BLOCK(render_group_draw);   

    f32 null_pixels_to_meters = 1.0f;
    
    for (u32 base_address = 0;
         base_address < group->push_buffer_size;
         ) {
        Render_Entry_Header *header = (Render_Entry_Header *)(group->push_buffer_base + base_address);
        base_address += sizeof(*header);
        void *data = (u8 *)header + sizeof(*header);
        
        switch (header->type) {
            case RenderEntryType_Render_Entry_Clear: {
                Render_Entry_Clear *entry = (Render_Entry_Clear *)data;
                draw_rect(output_target,
                          make_vec2(0.0f),
                          make_vec2((f32)output_target->width, (f32)output_target->height),
                          entry->color, clip_rect, even);
                
                base_address += sizeof(*entry);
            } break;
                
            case RenderEntryType_Render_Entry_Rect: {
                Render_Entry_Rect *entry = (Render_Entry_Rect *)data;
                draw_rect(output_target, entry->pos, entry->pos + entry->size, entry->color, clip_rect, even);
                
                base_address += sizeof(*entry);
            } break;
                
            case RenderEntryType_Render_Entry_Bitmap: {
                Render_Entry_Bitmap *entry = (Render_Entry_Bitmap *)data;
                assert(entry->bitmap);
#if 0
                draw_bitmap(output_target, entry->bitmap, entry->pos, entry->color.a);
#else
                draw_rect_quickly(output_target, entry->pos,
                                  make_vec2(entry->size.x, 0.0f), make_vec2(0.0f, entry->size.y),
                                  entry->color, entry->bitmap, null_pixels_to_meters, clip_rect, even);
#endif
                
                base_address += sizeof(*entry);
            } break;
                
            case RenderEntryType_Render_Entry_Coordinate_System: {
                Render_Entry_Coordinate_System *entry = (Render_Entry_Coordinate_System *)data;
#if 0
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
#endif
                
                base_address += sizeof(*entry);
            } break;
            {
                invalid_default_case;
            }
        }
    }
    
    END_TIMED_BLOCK(render_group_draw);
}

struct Tile_Render_Work {
    Render_Group *render_group;
    Bitmap *output_target;
    Rect2i clip_rect;
};

internal
OS_WORK_QUEUE_CALLBACK(render_group_draw_work) {
    Tile_Render_Work *work = (Tile_Render_Work *)data;
    render_group_draw(work->render_group, work->output_target, work->clip_rect, false);
    render_group_draw(work->render_group, work->output_target, work->clip_rect, true);
}

internal void
render_group_draw_not_tiled(Render_Group *render_group, Bitmap *output_target) {
    assert((((uintptr_t)output_target->memory) & 15) == 0);
                
    Rect2i clip_rect;
    clip_rect.min_x = 0;
    clip_rect.min_y = 0;
    clip_rect.max_x = output_target->width;
    clip_rect.max_y = output_target->height;

    Tile_Render_Work work;
    work.render_group = render_group;
    work.output_target = output_target;
    work.clip_rect = clip_rect;

    render_group_draw_work(0, &work);
}

internal void
render_group_draw_tiled(OS_Work_Queue *render_queue, Render_Group *render_group, Bitmap *output_target) {
    // TODO(xkazu0x):
    // - Make sure that tiles are all cache-aligned
    // - Can we get hyperthreads synced so they do interleaved lines?
    // - How big should the tiles be for performance?
    // - Actually ballpark the memory bandwidth for out draw_rect_quickly
    // - Re-test some of our instruction choices
    
    const s32 tile_count_x = 4;
    const s32 tile_count_y = 4;
    Tile_Render_Work work_array[tile_count_x*tile_count_y];
    
    assert((((uintptr_t)output_target->memory) & 15) == 0);
    s32 tile_width = output_target->width/tile_count_x;
    s32 tile_height = output_target->height/tile_count_y;

    tile_width = ((tile_width + 3)/4)*4;
    
    u32 work_count = 0;    
    for (s32 tile_y = 0;
         tile_y < tile_count_y;
         ++tile_y) {
        for (s32 tile_x = 0;
             tile_x < tile_count_x;
             ++tile_x) {
            Tile_Render_Work *work = work_array + work_count++;
            
            Rect2i clip_rect;
            clip_rect.min_x = tile_x*tile_width;
            clip_rect.min_y = tile_y*tile_height;
            clip_rect.max_x = clip_rect.min_x + tile_width;
            clip_rect.max_y = clip_rect.min_y + tile_height;

            if (tile_x < (tile_count_x - 1)) {
                clip_rect.max_x = output_target->width;
            }
            
            if (tile_y < (tile_count_y - 1)) {
                clip_rect.max_y = output_target->height;
            }
            
            work->render_group = render_group;
            work->output_target = output_target;
            work->clip_rect = clip_rect;

            os_work_queue_add_entry(render_queue, render_group_draw_work, work);
        }
    }

    os_work_queue_complete(render_queue);
}

internal void
render_perspective(Render_Group *group, s32 pixel_width, s32 pixel_height, f32 meters_to_pixels, f32 focal_length, f32 distance_above_target) {
    // TODO(xkazu0x): Need to adjust this based on the buffer size
    f32 pixels_to_meters = safe_ratio1(1.0f, meters_to_pixels);
    group->monitor_half_dim_in_meters = 0.5f*make_vec2((f32)pixel_width, (f32)pixel_height)*pixels_to_meters;

    group->transform.meters_to_pixels = meters_to_pixels;
    group->transform.focal_length = focal_length; // NOTE(xkazu0x): Meters the person is sitting from the monitor
    group->transform.distance_above_target = distance_above_target;
    group->transform.near_clip_plane = 0.2f;
    group->transform.screen_center = 0.5f*make_vec2((f32)pixel_width, (f32)pixel_height);

    group->transform.orthographic = false;
}

internal void
render_orthographic(Render_Group *group, s32 pixel_width, s32 pixel_height, f32 meters_to_pixels) {
    // TODO(xkazu0x): Need to adjust this based on the buffer size
    f32 pixels_to_meters = safe_ratio1(1.0f, meters_to_pixels);
    group->monitor_half_dim_in_meters = 0.5f*make_vec2((f32)pixel_width, (f32)pixel_height)*pixels_to_meters;
    
    group->transform.meters_to_pixels = meters_to_pixels;
    group->transform.focal_length = 1.0f; // NOTE(xkazu0x): Meters the person is sitting from the monitor
    group->transform.distance_above_target = 1.0f;
    group->transform.near_clip_plane = 0.2f;
    group->transform.screen_center = 0.5f*make_vec2((f32)pixel_width, (f32)pixel_height);

    group->transform.orthographic = true;
}

#define render_push(group, type) (type *)render_push_(group, sizeof(type), RenderEntryType_##type)
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
render_rect(Render_Group *group, Vec3 offset, Vec2 size, Vec4 color = make_vec4(1.0f)) {
    Vec3 pos = offset - make_vec3(0.5f*size, 0.0f);

    Project_Point_Result projection = project_point(&group->transform, pos);
    if (projection.valid) {
        Render_Entry_Rect *entry = render_push(group, Render_Entry_Rect);
        if (entry) {
            entry->pos = projection.point;
            entry->size = projection.scale*size;
            entry->color = color;
        }
    }
}

internal void
render_rect_outline(Render_Group *group, Vec3 offset, Vec2 size, Vec4 color = make_vec4(1.0f)) {
    f32 thickness = 0.2f;
    
    // NOTE(xkazu0x): top and bottom
    render_rect(group, offset - make_vec3(0.0f, 0.5f*size.y, 0.0f), make_vec2(size.x, thickness), color);
    render_rect(group, offset + make_vec3(0.0f, 0.5f*size.y, 0.0f), make_vec2(size.x, thickness), color);
    
    // NOTE(xkazu0x): left and right
    render_rect(group, offset - make_vec3(0.5f*size.x, 0.0f, 0.0f), make_vec2(thickness, size.y), color);
    render_rect(group, offset + make_vec3(0.5f*size.x, 0.0f, 0.0f), make_vec2(thickness, size.y), color);
}

internal void
render_bitmap(Render_Group *group, Bitmap *bitmap, Vec3 offset, f32 height, Vec4 color = make_vec4(1.0f)) {
    Vec2 size = make_vec2(height*bitmap->width_over_height, height);
    Vec2 align = hadamard_product(bitmap->align_percentage, size);
    Vec3 pos = offset - make_vec3(align, 0.0f);
    
    Project_Point_Result projection = project_point(&group->transform, pos);
    if (projection.valid) {    
        Render_Entry_Bitmap *entry = render_push(group, Render_Entry_Bitmap);
        if (entry) {
            entry->bitmap = bitmap;
            entry->pos = projection.point;
            entry->size = projection.scale*size;
            entry->color = group->global_alpha*color;
        }
    }
}

// TODO(xkazu0x): put this function in the header
internal void
render_bitmap(Render_Group *group, Bitmap_ID id, Vec3 offset, f32 height, Vec4 color = make_vec4(1.0f)) {
    Bitmap *bitmap = asset_get_bitmap(group->asset_manager, id);
    if (bitmap) {
        render_bitmap(group, bitmap, offset, height, color);
    } else {
        asset_load_bitmap(group->asset_manager, id);
        ++group->missing_resource_count;
    }
}

//
// TODO(xkazu0x): to be defined.
//

internal void
render_coordinate_system(Render_Group *group,
                         Vec2 origin, Vec2 x_axis, Vec2 y_axis,
                         Vec4 color, Bitmap *texture, Bitmap *normal_map,
                         Environment_Map *top, Environment_Map *middle, Environment_Map *bottom) {
#if 0
    Entity_Basis_Point basis = get_entity_basis_point(group, &entry->entity_basis, screen_dim);
    if (basis.valid) {
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
#endif
}

internal Vec2
unproject(Render_Group *group, Vec2 projected_xy, f32 at_distance_from_camera) {
    Vec2 world_xy = (at_distance_from_camera / group->transform.focal_length)*projected_xy;
    return(world_xy);
}

internal Rect2
get_camera_rect_at_distance(Render_Group *group, f32 distance_from_camera) {
    Vec2 raw_point = unproject(group, group->monitor_half_dim_in_meters, distance_from_camera);
    Rect2 result = make_rect2_center_half_dim(make_vec2(0.0f), raw_point);
    return(result);
}

internal Rect2
get_camera_rect_at_target(Render_Group *group) {
    Rect2 result = get_camera_rect_at_distance(group, group->transform.distance_above_target);
    return(result);
}

internal b32
all_resources_present(Render_Group *group) {
    b32 result = (group->missing_resource_count == 0);
    return(result);
}
