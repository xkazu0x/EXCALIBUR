#ifndef EXCALIBUR_MATH_H
#define EXCALIBUR_MATH_H

////////////////////////////////
// NOTE(xkazu0x): Math functions

#include <math.h>
#define sqrt_f32(x) sqrtf(x)
#define cbrt_f32(x) cbrtf(x)
#define ceil_f32(x) ceilf(x)
#define floor_f32(x) floorf(x)
#define round_f32(x) roundf(x)
#define abs_f32(x) fabsf(x)
#define sin_f32(x) sinf(x)
#define cos_f32(x) cosf(x)
#define tan_f32(x) tanf(x)
#define atan2_f32(y, x) atan2f(y, x)

#define square(x) ((x)*(x))
#define square_root(x) sqrt_f32(x)

////////////////////////////////
// NOTE(xkazu0x): Intrinsics

internal inline s32 truncate_f32_to_s32(f32 x);
internal inline u32 truncate_f32_to_u32(f32 x);

internal inline s32 sign_of(s32 x);
internal inline f32 sign_of(f32 x);

internal inline u32 rotate_left(u32 value, s32 amount);
internal inline u32 rotate_right(u32 value, s32 amount);

struct Bit_Scan {
    b32 found;
    u32 index;
};

internal inline Bit_Scan find_least_significant_set_bit(u32 value);

////////////////////////////////
// NOTE(xkazu0x): Vector types

union Vec2 {
    struct {
        f32 x, y;
    };
    struct {
        f32 u, v;
    };
    
    f32 e[2];
};

union Vec3 {
    struct {
        f32 x, y, z;
    };
    struct {
        f32 u, v, w;
    };
    struct {
        f32 r, g, b;
    };
    
    struct {
        Vec2 xy;
        f32 _z0;
    };
    struct {
        Vec2 uv;
        f32 _w0;
    };
    struct {
        Vec2 rg;
        f32 _b0;
    };
    
    f32 e[3];
};

union Vec4 {
    struct {
        f32 x, y, z, w;
    };
    struct {
        f32 r, g, b, a;
    };
    
    struct {
        Vec2 xy;
        Vec2 zw;
    };
    struct {
        Vec2 rg;
        Vec2 ba;
    };

    struct {
        Vec3 xyz;
        f32 _w0;
    };
    struct {
        Vec3 rgb;
        f32 _a0;
    };
    
    f32 e[4];
};

////////////////////////////////
// NOTE(xkazu0x): Vector functions

internal inline Vec2 make_vec2(f32 f);
internal inline Vec3 make_vec3(f32 f);
internal inline Vec4 make_vec4(f32 f);

internal inline Vec2 make_vec2(f32 x, f32 y);
internal inline Vec3 make_vec3(f32 x, f32 y, f32 z);
internal inline Vec4 make_vec4(f32 x, f32 y, f32 z, f32 w);

internal inline Vec3 make_vec3(Vec2 xy, f32 z);
internal inline Vec4 make_vec4(Vec2 xy, f32 z, f32 w);
internal inline Vec4 make_vec4(Vec3 xyz, f32 w);

internal inline Vec2 operator+(Vec2 a, Vec2 b);
internal inline Vec3 operator+(Vec3 a, Vec3 b);
internal inline Vec4 operator+(Vec4 a, Vec4 b);

internal inline Vec2 &operator+=(Vec2 &a, Vec2 b);
internal inline Vec3 &operator+=(Vec3 &a, Vec3 b);
internal inline Vec4 &operator+=(Vec4 &a, Vec4 b);

internal inline Vec2 operator-(Vec2 v);
internal inline Vec3 operator-(Vec3 v);
internal inline Vec4 operator-(Vec4 v);

internal inline Vec2 operator-(Vec2 a, Vec2 b);
internal inline Vec3 operator-(Vec3 a, Vec3 b);
internal inline Vec4 operator-(Vec4 a, Vec4 b);

internal inline Vec2 &operator-=(Vec2 &a, Vec2 b);
internal inline Vec3 &operator-=(Vec3 &a, Vec3 b);
internal inline Vec4 &operator-=(Vec4 &a, Vec4 b);

internal inline Vec2 operator*(f32 s, Vec2 v);
internal inline Vec3 operator*(f32 s, Vec3 v);
internal inline Vec4 operator*(f32 s, Vec4 v);

internal inline Vec2 operator*(Vec2 v, f32 s);
internal inline Vec3 operator*(Vec3 v, f32 s);
internal inline Vec4 operator*(Vec4 v, f32 s);

internal inline Vec2 &operator*=(Vec2 &v, f32 s);
internal inline Vec3 &operator*=(Vec3 &v, f32 s);
internal inline Vec4 &operator*=(Vec4 &v, f32 s);

internal inline Vec2 operator/(Vec2 a, Vec2 b);
internal inline Vec3 operator/(Vec3 a, Vec3 b);
internal inline Vec4 operator/(Vec4 a, Vec4 b);

internal inline b32 operator==(Vec2 a, Vec2 b);
internal inline b32 operator==(Vec3 a, Vec3 b);
internal inline b32 operator==(Vec4 a, Vec4 b);

internal inline b32 operator!=(Vec2 a, Vec2 b);
internal inline b32 operator!=(Vec3 a, Vec3 b);
internal inline b32 operator!=(Vec4 a, Vec4 b);

internal inline Vec2 hadamard_product(Vec2 a, Vec2 b);
internal inline Vec3 hadamard_product(Vec3 a, Vec3 b);
internal inline Vec4 hadamard_product(Vec4 a, Vec4 b);

internal inline f32 dot_product(Vec2 a, Vec2 b);
internal inline f32 dot_product(Vec3 a, Vec3 b);
internal inline f32 dot_product(Vec4 a, Vec4 b);

internal inline f32 length_squared(Vec2 v);
internal inline f32 length_squared(Vec3 v);
internal inline f32 length_squared(Vec4 v);

internal inline f32 length(Vec2 v);
internal inline f32 length(Vec3 v);
internal inline f32 length(Vec4 v);

internal inline Vec2 normalize(Vec2 v);
internal inline Vec3 normalize(Vec3 v);
internal inline Vec4 normalize(Vec4 v);

internal inline Vec2 perp(Vec2 v);

////////////////////////////////
// NOTE(xkazu0x): Rectangle types

struct Rect2 {
    Vec2 min;
    Vec2 max;
};

struct Rect3 {
    Vec3 min;
    Vec3 max;
};

struct Rect2i {
    s32 min_x;
    s32 min_y;
    s32 max_x;
    s32 max_y;
};

internal inline Rect2i
make_rect2i(s32 min, s32 max) {
    Rect2i result;
    result.min_x = min;
    result.min_y = min;
    result.max_x = max;
    result.max_y = max;
    return(result);
}

internal inline Rect2i
make_rect2i(s32 min_x, s32 min_y, s32 max_x, s32 max_y) {
    Rect2i result;
    result.min_x = min_x;
    result.min_y = min_y;
    result.max_x = max_x;
    result.max_y = max_y;
    return(result);
}

internal inline Rect2i
rect2i_intersect(Rect2i a, Rect2i b) {
    Rect2i result;
    result.min_x = (a.min_x < b.min_x) ? b.min_x : a.min_x;
    result.min_y = (a.min_y < b.min_y) ? b.min_y : a.min_y;
    result.max_x = (a.max_x > b.max_x) ? b.max_x : a.max_x;
    result.max_y = (a.max_y > b.max_y) ? b.max_y : a.max_y;
    return(result);
}

internal inline Rect2i
rect2i_union(Rect2i a, Rect2i b) {
    Rect2i result;
    result.min_x = (a.min_x < b.min_x) ? a.min_x : b.min_x;
    result.min_y = (a.min_y < b.min_y) ? a.min_y : b.min_y;
    result.max_x = (a.max_x > b.max_x) ? a.max_x : b.max_x;
    result.max_y = (a.max_y > b.max_y) ? a.max_y : b.max_y;
    return(result);
}

internal inline s32
get_rect2i_clamped_area(Rect2i a) {
    s32 result = 0;
    
    s32 width = (a.max_x - a.min_x);
    s32 height = (a.max_y - a.min_y);

    if ((width > 0) && (height > 0)) {
        result = width*height;
    }
    
    return(result);
}

internal inline b32
has_rect2i_area(Rect2i a) {
    b32 result = ((a.min_x < a.max_x) && (a.min_y < a.max_y));
    return(result);
}

internal inline Rect2i
rect2i_inverted_infinity(void) {
    Rect2i result;
    result.min_x = result.min_y = s32_max;
    result.max_x = result.max_y = -s32_max;
    return(result);
}

////////////////////////////////
// NOTE(xkazu0x): Rectangle functions

internal inline Rect2 make_rect2_min_max(Vec2 min, Vec2 max);
internal inline Rect3 make_rect3_min_max(Vec3 min, Vec3 max);

internal inline Rect2 make_rect2_min_dim(Vec2 min, Vec2 dim);
internal inline Rect3 make_rect3_min_dim(Vec3 min, Vec3 dim);

internal inline Rect2 make_rect2_center_half_dim(Vec2 center, Vec2 half_dim);
internal inline Rect3 make_rect3_center_half_dim(Vec3 center, Vec3 half_dim);

internal inline Rect2 make_rect2_center_dim(Vec2 center, Vec2 dim);
internal inline Rect3 make_rect3_center_dim(Vec3 center, Vec3 dim);

internal inline Vec2 get_rect_min(Rect2 rect);
internal inline Vec3 get_rect_min(Rect3 rect);

internal inline Vec2 get_rect_max(Rect2 rect);
internal inline Vec3 get_rect_max(Rect3 rect);

internal inline Vec2 get_rect_dim(Rect2 rect);
internal inline Vec3 get_rect_dim(Rect3 rect);

internal inline Vec2 get_rect_center(Rect2 rect);
internal inline Vec3 get_rect_center(Rect3 rect);

internal inline Rect2 rect_add_radius(Rect2 rect, Vec2 radiuse);
internal inline Rect3 rect_add_radius(Rect3 rect, Vec3 radius);

internal inline Rect2 rect_offset(Rect2 rect, Vec2 offset);
internal inline Rect3 rect_offset(Rect3 rect, Vec3 offset);

internal inline b32 is_in_rect(Rect2 rect, Vec2 test);
internal inline b32 is_in_rect(Rect3 rect, Vec3 test);

internal inline b32 rect_intersect(Rect3 a, Rect3 b);

//
// TODO(xkazu0x): To be defined!
//

internal inline f32 lerp(f32 a, f32 t, f32 b);
internal inline Vec3 lerp(Vec3 a, f32 t, Vec3 b);
internal inline Vec4 lerp(Vec4 a, f32 t, Vec4 b);

internal inline f32 clamp(f32 min, f32 value, f32 max);
internal inline f32 clamp01(f32 value);
internal inline Vec2 clamp01(Vec2 value);
internal inline Vec3 clamp01(Vec3 value);

internal inline f32 safe_ratio(f32 numerator, f32 divisor, f32 n);
internal inline f32 safe_ratio0(f32 numerator, f32 divisor);
internal inline f32 safe_ratio1(f32 numerator, f32 divisor);

internal inline Vec2 get_barycentric(Rect2 rect, Vec2 point);
internal inline Vec3 get_barycentric(Rect3 rect, Vec3 point);

internal inline Vec3 make_rgb(f32 r, f32 g, f32 b);
internal inline Vec4 make_rgba(f32 r, f32 g, f32 b, f32 a);

#endif // EXCALIBUR_MATH_H
