////////////////////////////////
// NOTE(xkazu0x): Intrinsics

internal inline s32
truncate_f32_to_s32(f32 x) {
    s32 result = (s32)x;
    return(result);
}

internal inline u32
truncate_f32_to_u32(f32 x) {
    u32 result = (u32)x;
    return(result);
}

internal inline s32
sign_of(s32 x) {
    s32 result = (x >= 0) ? 1 : -1;
    return(result);
}

internal inline f32
sign_of(f32 x) {
    f32 result = (x >= 0.0f) ? 1.0f : -1.0f;
    return(result);
}

internal inline u32
rotate_left(u32 value, s32 amount) {
#if COMPILER_CL
    u32 result = _rotl(value, amount);
#else
    // TODO(xkazu0x): port this to other compiler platform
    amount &= 31;
    u32 result = ((value << amount) | (value >> (32 - amount)));
#endif
    return(result);
}

internal inline u32
rotate_right(u32 value, s32 amount) {
#if COMPILER_CL
    u32 result = _rotr(value, amount);
#else
    // TODO(xkazu0x): port this to other compiler platform
    amount &= 31;
    u32 result = ((value >> amount) | (value << (32 - amount)));
#endif
    return(result);
}

#if COMPILER_MSVC
#include <intrin.h>
#endif

internal inline Bit_Scan
find_least_significant_set_bit(u32 value) {
    Bit_Scan result = {};
#if COMPILER_CL
    result.found = _BitScanForward((unsigned long *)&result.index, value);
#else
    for (u32 test = 0;
         test < 32;
         ++test) {
        if (value & (1 << test)) {
            result.index = test;
            result.found = true;
            break;
        }
    }
#endif
    return(result);
}

////////////////////////////////
// NOTE(xkazu0x): Vector functions

internal inline Vec2
make_vec2(f32 f) {
    Vec2 result;
    result.x = f;
    result.y = f;
    return(result);
}

internal inline Vec3
make_vec3(f32 f) {
    Vec3 result;
    result.x = f;
    result.y = f;
    result.z = f;
    return(result);
}

internal inline Vec4
make_vec4(f32 f) {
    Vec4 result;
    result.x = f;
    result.y = f;
    result.z = f;
    result.w = f;
    return(result);
}

internal inline Vec2
make_vec2(f32 x, f32 y) {
    Vec2 result;
    result.x = x;
    result.y = y;
    return(result);
}

internal inline Vec3
make_vec3(f32 x, f32 y, f32 z) {
    Vec3 result;
    result.x = x;
    result.y = y;
    result.z = z;
    return(result);
}

internal inline Vec4
make_vec4(f32 x, f32 y, f32 z, f32 w) {
    Vec4 result;
    result.x = x;
    result.y = y;
    result.z = z;
    result.w = w;
    return(result);
}

internal inline Vec3
make_vec3(Vec2 xy, f32 z) {
    Vec3 result;
    result.xy = xy;
    result.z = z;
    return(result);
}

internal inline Vec4
make_vec4(Vec2 xy, f32 z, f32 w) {
    Vec4 result;
    result.xy = xy;
    result.z = z;
    result.w = w;
    return(result);
}

internal inline Vec4
make_vec4(Vec3 xyz, f32 w) {
    Vec4 result;
    result.xyz = xyz;
    result.w = w;
    return(result);
}

internal inline Vec2
operator+(Vec2 a, Vec2 b) {
    Vec2 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    return(result);
}

internal inline Vec3
operator+(Vec3 a, Vec3 b) {
    Vec3 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    return(result);
}

internal inline Vec4
operator+(Vec4 a, Vec4 b) {
    Vec4 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    result.w = a.w + b.w;
    return(result);
}

internal inline Vec2 &
operator+=(Vec2 &a, Vec2 b) {
    a = a + b;
    return(a);
}

internal inline Vec3 &
operator+=(Vec3 &a, Vec3 b) {
    a = a + b;
    return(a);
}

internal inline Vec4 &
operator+=(Vec4 &a, Vec4 b) {
    a = a + b;
    return(a);
}

internal inline Vec2
operator-(Vec2 v) {
    Vec2 result;
    result.x = -v.x;
    result.y = -v.y;
    return(result);
}

internal inline Vec3
operator-(Vec3 v) {
    Vec3 result;
    result.x = -v.x;
    result.y = -v.y;
    result.z = -v.z;
    return(result);
}

internal inline Vec4
operator-(Vec4 v) {
    Vec4 result;
    result.x = -v.x;
    result.y = -v.y;
    result.z = -v.z;
    result.w = -v.w;
    return(result);
}

internal inline Vec2
operator-(Vec2 a, Vec2 b) {
    Vec2 result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    return(result);
}

internal inline Vec3
operator-(Vec3 a, Vec3 b) {
    Vec3 result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;
    return(result);
}

internal inline Vec4
operator-(Vec4 a, Vec4 b) {
    Vec4 result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;
    result.w = a.w - b.w;
    return(result);
}

internal inline Vec2 &
operator-=(Vec2 &a, Vec2 b) {
    a = a - b;
    return(a);
}

internal inline Vec3 &
operator-=(Vec3 &a, Vec3 b) {
    a = a - b;
    return(a);
}

internal inline Vec4 &
operator-=(Vec4 &a, Vec4 b) {
    a = a - b;
    return(a);
}

internal inline Vec2
operator*(f32 s, Vec2 v) {
    Vec2 result;
    result.x = s*v.x;
    result.y = s*v.y;
    return(result);
}

internal inline Vec3
operator*(f32 s, Vec3 v) {
    Vec3 result;
    result.x = s*v.x;
    result.y = s*v.y;
    result.z = s*v.z;
    return(result);
}

internal inline Vec4
operator*(f32 s, Vec4 v) {
    Vec4 result;
    result.x = s*v.x;
    result.y = s*v.y;
    result.z = s*v.z;
    result.w = s*v.w;
    return(result);
}

internal inline Vec2
operator*(Vec2 v, f32 s) {
    Vec2 result;
    result.x = v.x*s;
    result.y = v.y*s;
    return(result);
}

internal inline Vec3
operator*(Vec3 v, f32 s) {
    Vec3 result;
    result.x = v.x*s;
    result.y = v.y*s;
    result.z = v.z*s;
    return(result);
}

internal inline Vec4
operator*(Vec4 v, f32 s) {
    Vec4 result;
    result.x = v.x*s;
    result.y = v.y*s;
    result.z = v.z*s;
    result.w = v.w*s;
    return(result);
}

internal inline Vec2 &
operator*=(Vec2 &v, f32 s) {
    v = v*s;
    return(v);
}

internal inline Vec3 &
operator*=(Vec3 &v, f32 s) {
    v = v*s;
    return(v);
}

internal inline Vec4 &
operator*=(Vec4 &v, f32 s) {
    v = v*s;
    return(v);
}

internal inline Vec2
operator/(Vec2 a, Vec2 b) {
    Vec2 result;
    result.x = a.x/b.x;
    result.y = a.y/b.y;
    return(result);
}

internal inline Vec3
operator/(Vec3 a, Vec3 b) {
    Vec3 result;
    result.x = a.x/b.x;
    result.y = a.y/b.y;
    result.z = a.z/b.z;
    return(result);
}

internal inline Vec4
operator/(Vec4 a, Vec4 b) {
    Vec4 result;
    result.x = a.x/b.x;
    result.y = a.y/b.y;
    result.z = a.z/b.z;
    result.w = a.w/b.w;
    return(result);
}

internal inline b32
operator==(Vec2 a, Vec2 b) {
    b32 result = ((a.x == b.x) &&
                  (a.y == b.y));
    return(result);
}

internal inline b32
operator==(Vec3 a, Vec3 b) {
    b32 result = ((a.x == b.x) &&
                  (a.y == b.y) &&
                  (a.z == b.z));
    return(result);
}

internal inline b32
operator==(Vec4 a, Vec4 b) {
    b32 result = ((a.x == b.x) &&
                  (a.y == b.y) &&
                  (a.z == b.z) &&
                  (a.w == b.w));
    return(result);
}

internal inline b32
operator!=(Vec2 a, Vec2 b) {
    b32 result = ((a.x != b.x) &&
                  (a.y != b.y));
    return(result);
}

internal inline b32
operator!=(Vec3 a, Vec3 b) {
    b32 result = ((a.x != b.x) &&
                  (a.y != b.y) &&
                  (a.z != b.z));
    return(result);
}

internal inline b32
operator!=(Vec4 a, Vec4 b) {
    b32 result = ((a.x != b.x) &&
                  (a.y != b.y) &&
                  (a.z != b.z) &&
                  (a.w != b.w));
    return(result);
}

internal inline Vec2
hadamard_product(Vec2 a, Vec2 b) {
    Vec2 result;
    result.x = a.x*b.x;
    result.y = a.y*b.y;
    return(result);
}

internal inline Vec3
hadamard_product(Vec3 a, Vec3 b) {
    Vec3 result;
    result.x = a.x*b.x;
    result.y = a.y*b.y;
    result.z = a.z*b.z;
    return(result);
}

internal inline Vec4
hadamard_product(Vec4 a, Vec4 b) {
    Vec4 result;
    result.x = a.x*b.x;
    result.y = a.y*b.y;
    result.z = a.z*b.z;
    result.w = a.w*b.w;
    return(result);
}

internal inline f32
dot_product(Vec2 a, Vec2 b) {
    f32 result = a.x*b.x + a.y*b.y;
    return(result);
}

internal inline f32
dot_product(Vec3 a, Vec3 b) {
    f32 result = a.x*b.x + a.y*b.y + a.z*b.z;
    return(result);
}

internal inline f32
dot_product(Vec4 a, Vec4 b) {
    f32 result = a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w;
    return(result);
}

internal inline f32
length_squared(Vec2 v) {
    f32 result = dot_product(v, v);
    return(result);
}

internal inline f32
length_squared(Vec3 v) {
    f32 result = dot_product(v, v);
    return(result);
}

internal inline f32
length_squared(Vec4 v) {
    f32 result = dot_product(v, v);
    return(result);
}

internal inline f32
length(Vec2 v) {
    f32 result = sqrt_f32(length_squared(v));
    return(result);
}

internal inline f32
length(Vec3 v) {
    f32 result = sqrt_f32(length_squared(v));
    return(result);
}

internal inline f32
length(Vec4 v) {
    f32 result = sqrt_f32(length_squared(v));
    return(result);
}

internal inline Vec2
normalize(Vec2 v) {
    Vec2 result = v*(1.0f/length(v));
    return(result);
}

internal inline Vec3
normalize(Vec3 v) {
    Vec3 result = v*(1.0f/length(v));
    return(result);
}

internal inline Vec4
normalize(Vec4 v) {
    Vec4 result = v*(1.0f/length(v));
    return(result);
}

internal inline Vec2
perp(Vec2 v) {
    Vec2 result = {-v.y, v.x};
    return(result);
}

////////////////////////////////
// NOTE(xkazu0x): Rectangle functions

internal inline Rect2
make_rect2_min_max(Vec2 min, Vec2 max) {
    Rect2 result;
    result.min = min;
    result.max = max;
    return(result);
}

internal inline Rect3
make_rect3_min_max(Vec3 min, Vec3 max) {
    Rect3 result;
    result.min = min;
    result.max = max;
    return(result);
}

internal inline Rect2
make_rect2_min_dim(Vec2 min, Vec2 dim) {
    Rect2 result;
    result.min = min;
    result.max = min + dim;
    return(result);
}

internal inline Rect3
make_rect3_min_dim(Vec3 min, Vec3 dim) {
    Rect3 result;
    result.min = min;
    result.max = min + dim;
    return(result);
}

internal inline Rect2
make_rect2_center_half_dim(Vec2 center, Vec2 half_dim) {
    Rect2 result;
    result.min = center - half_dim;
    result.max = center + half_dim;
    return(result);
}

internal inline Rect3
make_rect3_center_half_dim(Vec3 center, Vec3 half_dim) {
    Rect3 result;
    result.min = center - half_dim;
    result.max = center + half_dim;
    return(result);
}

internal inline Rect2
make_rect2_center_dim(Vec2 center, Vec2 dim) {
    Rect2 result = make_rect2_center_half_dim(center, 0.5f*dim);
    return(result);
}

internal inline Rect3
make_rect3_center_dim(Vec3 center, Vec3 dim) {
    Rect3 result = make_rect3_center_half_dim(center, 0.5f*dim);
    return(result);
}

internal inline Vec2
get_rect_min(Rect2 rect) {
    Vec2 result = rect.min;
    return(result);
}

internal inline Vec3
get_rect_min(Rect3 rect) {
    Vec3 result = rect.min;
    return(result);
}

internal inline Vec2
get_rect_max(Rect2 rect) {
    Vec2 result = rect.max;
    return(result);
}

internal inline Vec3
get_rect_max(Rect3 rect) {
    Vec3 result = rect.max;
    return(result);
}

internal inline Vec2
get_rect_dim(Rect2 rect) {
    Vec2 result = rect.max - rect.min;
    return(result);
}

internal inline Vec3
get_rect_dim(Rect3 rect) {
    Vec3 result = rect.max - rect.min;
    return(result);
}

internal inline Vec2
get_rect_center(Rect2 rect) {
    Vec2 result = 0.5f*(rect.min + rect.max);
    return(result);
}

internal inline Vec3
get_rect_center(Rect3 rect) {
    Vec3 result = 0.5f*(rect.min + rect.max);
    return(result);
}

internal inline Rect2
rect_add_radius(Rect2 rect, Vec2 radius) {
    Rect2 result;
    result.min = rect.min - radius;
    result.max = rect.max + radius;
    return(result);
}

internal inline Rect3
rect_add_radius(Rect3 rect, Vec3 radius) {
    Rect3 result;
    result.min = rect.min - radius;
    result.max = rect.max + radius;
    return(result);
}

internal inline Rect2
rect_offset(Rect2 rect, Vec2 offset) {
    Rect2 result;
    result.min = rect.min + offset;
    result.max = rect.max + offset;
    return(result);
}

internal inline Rect3
rect_offset(Rect3 rect, Vec3 offset) {
    Rect3 result;
    result.min = rect.min + offset;
    result.max = rect.max + offset;
    return(result);
}

internal inline b32
is_in_rect(Rect2 rect, Vec2 test) {
    b32 result = ((test.x >= rect.min.x) &&
                  (test.y >= rect.min.y) &&
                  (test.x < rect.max.x) &&
                  (test.y < rect.max.y));
    return(result);
}

internal inline b32
is_in_rect(Rect3 rect, Vec3 test) {
    b32 result = ((test.x >= rect.min.x) &&
                  (test.y >= rect.min.y) &&
                  (test.z >= rect.min.z) &&
                  (test.x < rect.max.x) &&
                  (test.y < rect.max.y) &&
                  (test.z < rect.max.z));
    return(result);
}

internal inline b32
rect_intersect(Rect3 a, Rect3 b) {
    b32 result = !((b.max.x <= a.min.x) ||
                   (b.min.x >= a.max.x) ||
                   (b.max.y <= a.min.y) ||
                   (b.min.y >= a.max.y) ||
                   (b.max.z <= a.min.z) ||
                   (b.min.z >= a.max.z));
    return(result);
}

//
// TODO(xkazu0x) To be defined!
//

internal inline f32
lerp(f32 a, f32 t, f32 b) {
    f32 result = (1.0f - t)*a + t*b;
    return(result);
}

internal inline Vec3
lerp(Vec3 a, f32 t, Vec3 b) {
    Vec3 result = (1.0f - t)*a + t*b;
    return(result);
}

internal inline Vec4
lerp(Vec4 a, f32 t, Vec4 b) {
    Vec4 result = (1.0f - t)*a + t*b;
    return(result);
}

internal inline f32
clamp(f32 min, f32 value, f32 max) {
    f32 result = value;
    if (result < min) {
        result = min;
    } else if (result > max) {
        result = max;
    }
    return(result);
}

internal inline f32
clamp01(f32 value) {
    f32 result = clamp(0.0f, value, 1.0f);
    return(result);
}

internal inline Vec2
clamp01(Vec2 value) {
    Vec2 result;
    result.x = clamp01(value.x);
    result.y = clamp01(value.y);
    return(result);
}

internal inline Vec3
clamp01(Vec3 value) {
    Vec3 result;
    result.x = clamp01(value.x);
    result.y = clamp01(value.y);
    result.z = clamp01(value.z);
    return(result);
}

// TODO(xkazu0x): Put function in the header
internal inline f32
clamp01_map_to_range(f32 min, f32 t, f32 max) {
    f32 result = 0;
    f32 range = max - min;
    if (range != 0.0f) result = clamp01((t - min) / range);
    return(result);
}

internal inline f32
safe_ratio(f32 numerator, f32 divisor, f32 n) {
    f32 result = n;
    if (divisor != 0.0f) {
        result = numerator/divisor;
    }
    return(result);
}

internal inline f32
safe_ratio0(f32 numerator, f32 divisor) {
    f32 result = safe_ratio(numerator, divisor, 0.0f);
    return(result);
}

internal inline f32
safe_ratio1(f32 numerator, f32 divisor) {
    f32 result = safe_ratio(numerator, divisor, 1.0f);
    return(result);
}

internal inline Vec2
get_barycentric(Rect2 rect, Vec2 point) {
    Vec2 result;
    result.x = safe_ratio0(point.x - rect.min.x, rect.max.x - rect.min.x);
    result.y = safe_ratio0(point.y - rect.min.y, rect.max.y - rect.min.y);
    return(result);
}

internal inline Vec3
get_barycentric(Rect3 rect, Vec3 point) {
    Vec3 result;
    result.x = safe_ratio0(point.x - rect.min.x, rect.max.x - rect.min.x);
    result.y = safe_ratio0(point.y - rect.min.y, rect.max.y - rect.min.y);
    result.z = safe_ratio0(point.z - rect.min.z, rect.max.z - rect.min.z);
    return(result);
}

internal inline Vec3
make_rgb(f32 r, f32 g, f32 b) {
    Vec3 result;
    result.r = r/255.0f;
    result.g = g/255.0f;
    result.b = b/255.0f;
    return(result);
}

internal inline Vec4
make_rgba(f32 r, f32 g, f32 b, f32 a) {
    Vec4 result;
    result.r = r/255.0f;
    result.g = g/255.0f;
    result.b = b/255.0f;
    result.a = a/255.0f;
    return(result);
}
