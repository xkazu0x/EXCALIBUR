#ifndef EXCALIBUR_RENDER_H
#define EXCALIBUR_RENDER_H

//
// NOTE(xkazu0x):
// 1. Everywhere outsided the renderer, Y _always_ goes upward, X to the right.
// 
// 2. All bitmaps including the render target are assumed to be bottom-up
//    (meaning that the first row pointer points to the bottom-most row
//     when viewed on screen).
// 
// 3. It is mandatory that all inputs to the renderer are in world
//    coordinate ("meters"), NOT pixels. If for some reason something
//    absolutely has to be specified in pixels, that will be explicitly
//    marked in the API, but this should occur exceedingly sparingly.
//
// 4. Z is a special coordinate because it is broken up into discrete slices.
//    And the renderer actually understands these slices. Z slices are what
//    control the scale of things, whereas Z offsets inside a slice are
//    what control Y offsetting.
//
// 5. All color values specified to the renderer as Vec4's are in
//    NON-premultiplied alpha.
//

struct Environment_Map {
    Bitmap lod[4]; // NOTE(xkazu0x): levels of detail
    f32 pos_z;
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
    Vec2 pos;
    Vec2 size;
    Vec4 color;
};

struct Render_Entry_Bitmap {
    Bitmap *bitmap;
    Vec2 pos;
    Vec2 size;
    Vec4 color;
};

// NOTE(xkazu0x): This is only for test
// {
struct Render_Entry_Coordinate_System {
    f32 pixels_to_meters; // TODO(xkazu0x): Need to store this for lighting!
    
    Vec2 origin;
    Vec2 x_axis;
    Vec2 y_axis;
    Vec4 color;
    Bitmap *texture;
    Bitmap *normal_map;
    Environment_Map *top;
    Environment_Map *middle;
    Environment_Map *bottom;
};
// }

struct Render_Transform {
    b32 orthographic;
    
    f32 meters_to_pixels; // NOTE(xkazu0x): This translates meters _on the monitor_ into pixels _on the monitor_
    Vec2 screen_center;
    
    // NOTE(xkazu0x): Camera parameters
    f32 focal_length;
    f32 distance_above_target;
    f32 near_clip_plane;

    Vec3 offset;
    f32 scale;
};

struct Render_Group {
    struct Asset_Manager *asset_manager;
    f32 global_alpha;
        
    Vec2 monitor_half_dim_in_meters;    
    Render_Transform transform;
    
    u32 max_push_buffer_size;
    u32 push_buffer_size;
    u8 *push_buffer_base;

    u32 missing_resource_count;
};

internal Vec4 srgb255_to_linear1(Vec4 color);
internal Vec4 linear1_to_srgb255(Vec4 color);


#endif // EXCALIBUR_RENDER_H
