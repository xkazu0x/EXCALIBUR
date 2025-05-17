#ifndef EXCALIBUR_RENDER_GROUP_H
#define EXCALIBUR_RENDER_GROUP_H

struct Render_Basis {
    Vec3 pos;
};

struct Render_Entity_Basis {
    Render_Basis *basis;
    Vec3 offset;
    f32 entity_zc;
};

// NOTE(xkazu0x): Render_Group_Entry_Type is a "compact discriminated union"
enum Render_Entry_Type {
    RenderEntryType_Render_Entry_Clear,
    RenderEntryType_Render_Entry_Bitmap,
    RenderEntryType_Render_Entry_Rect,
};

struct Render_Entry_Header {
    Render_Entry_Type type;
};

struct Render_Entry_Clear {
    Render_Entry_Header header;
    Vec4 color;
};

struct Render_Entry_Bitmap {
    Render_Entry_Header header;
    Render_Entity_Basis entity_basis;
    Bitmap *bitmap;
    Vec4 color;
};

struct Render_Entry_Rect {
    Render_Entry_Header header;
    Render_Entity_Basis entity_basis;
    Vec2 dim;
    Vec4 color;
};

struct Render_Group {
    Render_Basis *default_basis;
    f32 meters_to_pixels;
    
    u32 max_push_buffer_size;
    u32 push_buffer_size;
    u8 *push_buffer_base;
};

internal void draw_rect(Bitmap *buffer, Vec2 min, Vec2 max, Vec3 color);
internal void draw_rect_outline(Bitmap *buffer, Vec2 min, Vec2 max, Vec3 color, f32 r = 1.0f);
internal void draw_bitmap(Bitmap *buffer, Bitmap *bitmap, Vec2 offset, f32 c_alpha = 1.0f);

internal Render_Entry_Header *render_push_(Render_Group *group, u32 size, Render_Entry_Type type);
#define render_push(group, type) (type *)render_push_(group, sizeof(type), RenderEntryType_##type)

internal void push_entry(Render_Group *group, Bitmap *bitmap, Vec3 offset, Vec2 align, Vec4 color, f32 entity_zc = 1.0f);
internal void push_bitmap(Render_Group *group, Bitmap *bitmap, Vec3 offset, Vec2 align, f32 alpha = 1.0f, f32 entity_zc = 1.0f);
internal void push_rect(Render_Group *group, Vec3 offset, Vec2 dim, Vec4 color, f32 entity_zc = 1.0f);
internal void push_rect_outline(Render_Group *group, Vec3 offset, Vec2 dim, Vec4 color, f32 entity_zc = 1.0f);

internal Vec2 get_render_entity_basis_point(Render_Group *group, Render_Entity_Basis *entity_basis, Vec2 screen_center);

internal Render_Group *render_group_alloc(Arena *arena, u32 max_push_buffer_size, f32 meters_to_pixels);
internal void render_group_draw(Render_Group *group, Bitmap *output_target);

#endif // EXCALIBUR_RENDER_GROUP_H
