#ifndef EXCALIBUR_RENDER_H
#define EXCALIBUR_RENDER_H

struct Environment_Map {
    Bitmap *LOD[4]; // NOTE(xkazu0x): levels of detail
};

struct Render_Basis {
    Vec3 pos;
};

struct Render_Entity_Basis {
    Render_Basis *basis;
    Vec3 offset;
    f32 entity_zc;
};

enum Render_Entry_Type {
    RenderEntryType_Render_Entry_Clear,
    RenderEntryType_Render_Entry_Rect,
    RenderEntryType_Render_Entry_Bitmap,
    RenderEntryType_Render_Entry_Coordinate_System,
};

struct Render_Entry_Header {
    Render_Entry_Type type;
};

struct Render_Entry_Clear {
    Vec4 color;
};

struct Render_Entry_Rect {
    Render_Entity_Basis entity_basis;
    Vec2 dim;
    Vec4 color;
};

struct Render_Entry_Bitmap {
    Render_Entity_Basis entity_basis;
    Bitmap *bitmap;
    Vec4 color;
};

struct Render_Entry_Coordinate_System {
    Vec2 origin;
    Vec2 axis_x;
    Vec2 axis_y;
    Vec4 color;
    Bitmap *texture;
    Bitmap *normal_map;
    Environment_Map *top;
    Environment_Map *middle;
    Environment_Map *bottom;
};

struct Render_Group {
    Render_Basis *default_basis;
    f32 meters_to_pixels;
    
    u32 max_push_buffer_size;
    u32 push_buffer_size;
    u8 *push_buffer_base;
};

internal Render_Group *render_group_alloc(Arena *arena, u32 max_push_buffer_size, f32 meters_to_pixels);

internal void *render_push_(Render_Group *group, u32 size, Render_Entry_Type type);
#define render_push(group, type) (type *)render_push_(group, sizeof(type), RenderEntryType_##type)

internal void render_clear(Render_Group *group, Vec4 color);
internal void render_rect(Render_Group *group, Vec3 offset, Vec2 dim, Vec4 color, f32 entity_zc = 1.0f);
internal void render_rect_outline(Render_Group *group, Vec3 offset, Vec2 dim, Vec4 color, f32 entity_zc = 1.0f);
internal void render_bitmap(Render_Group *group, Bitmap *bitmap, Vec3 offset, Vec2 align, f32 alpha = 1.0f, f32 entity_zc = 1.0f);
//internal void render_coordinate_system(Render_Group *group, Vec2 origin, Vec2 axis_x, Vec2 axis_y, Vec4 color, Bitmap *texture, Bitmap *normal_map);

internal void render_group_draw(Render_Group *group, Bitmap *output_target);

#endif // EXCALIBUR_RENDER_H
