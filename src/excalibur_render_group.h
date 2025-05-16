#ifndef EXCALIBUR_RENDER_GROUP_H
#define EXCALIBUR_RENDER_GROUP_H

struct Render_Basis {
    Vec3 pos;
};

struct Entity_Visible_Piece {
    Render_Basis *basis;
    Bitmap *bitmap;
    Vec3 offset;
    
    Vec4 color;
    Vec2 size;
    
    f32 entity_zc;
};

struct Render_Group {
    Render_Basis *default_basis;
    f32 meters_to_pixels;
    u32 piece_count;
    
    u32 max_push_buffer_size;
    u32 push_buffer_size;
    u8 *push_buffer_base;
};

internal void *
render_push(Render_Group *group, u32 size) {
    void *result = 0;
    if ((group->push_buffer_size + size) < group->max_push_buffer_size) {
        result = (group->push_buffer_base + group->push_buffer_size);
        group->push_buffer_size += size;
    } else {
        InvalidPath;
    }
    return(result);
}

internal void
push_piece(Render_Group *group, Bitmap *bitmap,
           Vec3 offset, Vec2 size, Vec4 color, f32 entity_zc = 1.0f) {
    Entity_Visible_Piece *piece = (Entity_Visible_Piece *)render_push(group, sizeof(Entity_Visible_Piece));
    piece->basis = group->default_basis;
    piece->bitmap = bitmap;
    piece->offset = make_vec3(group->meters_to_pixels*offset.x,
                              group->meters_to_pixels*(-offset.y),
                              offset.z);
    piece->color = color;
    piece->size = size;
    piece->entity_zc = entity_zc;
}

internal void
push_bitmap(Render_Group *group, Bitmap *bitmap, Vec3 offset, Vec2 align,
            f32 alpha = 1.0f, f32 entity_zc = 1.0f) {
    push_piece(group, bitmap, offset, make_vec2(0.0f), make_vec4(make_vec3(1.0f), alpha), entity_zc);
}

internal void
push_rect(Render_Group *group, Vec3 offset, Vec2 size, Vec4 color,
          f32 entity_zc = 1.0f) {
    push_piece(group, 0, offset, size, color, entity_zc);
}

internal void
push_rect_outline(Render_Group *group, Vec3 offset, Vec2 size, Vec4 color,
                  f32 entity_zc = 1.0f) {
    f32 thickness = 0.2f;
    
    // NOTE(xkazu0x): top and bottom
    push_piece(group, 0, offset - make_vec3(0.0f, 0.5f*size.y, 0.0f), make_vec2(size.x, thickness), color, entity_zc);
    push_piece(group, 0, offset + make_vec3(0.0f, 0.5f*size.y, 0.0f), make_vec2(size.x, thickness), color, entity_zc);
    
    // NOTE(xkazu0x): left and right
    push_piece(group, 0, offset - make_vec3(0.5f*size.x, 0.0f, 0.0f), make_vec2(thickness, size.y), color, entity_zc);
    push_piece(group, 0, offset + make_vec3(0.5f*size.x, 0.0f, 0.0f), make_vec2(thickness, size.y), color, entity_zc);
}

#endif // EXCALIBUR_RENDER_GROUP_H
