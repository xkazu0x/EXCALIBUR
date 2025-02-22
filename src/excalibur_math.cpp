////////////////////////////////
// NOTE(xkazu0x): math functions
#include <math.h>

internal f32
abs_f32(f32 x) {
    union { f32 f; u32 u; } result;
    result.f = x;
    result.u &= 0x7fffffff;
    return(result.f);
}

internal f64
abs_f64(f64 x) {
    union { f64 f; u64 u; } result;
    result.f = x;
    result.u &= 0x7fffffffffffffff;
    return(result.f);
}

internal f32
sqrt_f32(f32 x) {
    return(sqrtf(x));
}

internal f32
sin_f32(f32 x) {
    return(sinf(x));
}

internal f32
cos_f32(f32 x) {
    return(cosf(x));
}

internal f32
tan_f32(f32 x) {
    return(tanf(x));
}

internal f64
sqrt_f64(f64 x) {
    return(sqrtf(x));
}

internal f64
sin_f64(f64 x) {
    return(sinf(x));
}

internal f64
cos_f64(f64 x) {
    return(cosf(x));
}

internal f64
tan_f64(f64 x) {
    return(tanf(x));
}

/////////////////////////////////////////
// NOTE(xkazu0x): compound type functions

internal inline vec2i
vec2i_create(s32 x, s32 y) {
    vec2i result;
    result.x = x;
    result.y = y;
    return(result);
}

internal inline vec2f
vec2f_create(f32 x, f32 y) {
    vec2f result;
    result.x = x;
    result.y = y;
    return(result);
}

internal inline vec3f
vec3f_create(f32 x, f32 y, f32 z) {
    vec3f result;
    result.x = x;
    result.y = y;
    result.z = z;
    return(result);
}

internal inline vec4f
vec4f_create(f32 x, f32 y, f32 z, f32 w) {
    vec4f result;
    result.x = x;
    result.y = y;
    result.z = z;
    result.w = w;
    return(result);
}

internal inline vec2i
operator+(vec2i a, vec2i b) {
    vec2i result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    return(result);
}

internal inline vec2f
operator+(vec2f a, vec2f b) {
    vec2f result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    return(result);
}

internal inline vec3f
operator+(vec3f a, vec3f b) {
    vec3f result;
    result.x = a.x + b.x;
    result.x = a.y + b.y;
    result.x = a.z + b.z;
    return(result);
}

internal inline vec4f
operator+(vec4f a, vec4f b) {
    vec4f result;
    result.x = a.x + b.x;
    result.x = a.y + b.y;
    result.x = a.z + b.z;
    result.x = a.w + b.w;
    return(result);
}

internal inline vec2i
operator-(vec2i a, vec2i b) {
    vec2i result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    return(result);
}

internal inline vec2f
operator-(vec2f a, vec2f b) {
    vec2f result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    return(result);
}

internal inline vec3f
operator-(vec3f a, vec3f b) {
    vec3f result;
    result.x = a.x - b.x;
    result.x = a.y - b.y;
    result.x = a.z - b.z;
    return(result);
}

internal inline vec4f
operator-(vec4f a, vec4f b) {
    vec4f result;
    result.x = a.x - b.x;
    result.x = a.y - b.y;
    result.x = a.z - b.z;
    result.x = a.w - b.w;
    return(result);
}

internal inline vec2i
operator*(vec2i v, s32 s) {
    vec2i result;
    result.x = v.x * s;
    result.y = v.y * s;
    return(result);
}

internal inline vec2f
operator*(vec2f v, f32 s) {
    vec2f result;
    result.x = v.x * s;
    result.y = v.y * s;
    return(result);
}

internal inline vec3f
operator*(vec3f v, f32 s) {
    vec3f result;
    result.x = v.x * s;
    result.y = v.y * s;
    result.z = v.z * s;
    return(result);
}

internal inline vec4f
operator*(vec4f v, f32 s) {
    vec4f result;
    result.x = v.x * s;
    result.y = v.y * s;
    result.z = v.z * s;
    result.w = v.w * s;
    return(result);
}

internal inline vec2i
operator*(s32 s, vec2i v) {
    vec2i result;
    result.x = v.x * s;
    result.y = v.y * s;
    return(result);
}

internal inline vec2f
operator*(f32 s, vec2f v) {
    vec2f result;
    result.x = v.x * s;
    result.y = v.y * s;
    return(result);
}

internal inline vec3f
operator*(f32 s, vec3f v) {
    vec3f result;
    result.x = v.x * s;
    result.y = v.y * s;
    result.z = v.z * s;
    return(result);
}

internal inline vec4f
operator*(f32 s, vec4f v) {
    vec4f result;
    result.x = v.x * s;
    result.y = v.y * s;
    result.z = v.z * s;
    result.w = v.w * s;
    return(result);
}

internal inline vec2i
operator/(vec2i v, s32 s) {
    vec2i result;
    result.x = v.x / s;
    result.y = v.y / s;
    return(result);
}

internal inline vec2f
operator/(vec2f v, f32 s) {
    vec2f result;
    result.x = v.x / s;
    result.y = v.y / s;
    return(result);
}

internal inline vec3f
operator/(vec3f v, f32 s) {
    vec3f result;
    result.x = v.x / s;
    result.y = v.y / s;
    result.z = v.z / s;
    return(result);
}

internal inline vec4f
operator/(vec4f v, f32 s) {
    vec4f result;
    result.x = v.x / s;
    result.y = v.y / s;
    result.z = v.z / s;
    result.w = v.w / s;
    return(result);
}

internal inline vec2f
vec2f_hadamard(vec2f a, vec2f b) {
    vec2f result;
    result.x = a.x * b.x;
    result.y = a.y * b.y;
    return(result);
}

internal inline vec3f
vec3f_hadamard(vec3f a, vec3f b) {
    vec3f result;
    result.x = a.x * b.x;
    result.y = a.y * b.y;
    result.z = a.z * b.z;
    return(result);
}

internal inline vec4f
vec4f_hadamard(vec4f a, vec4f b) {
    vec4f result;
    result.x = a.x * b.x;
    result.y = a.y * b.y;
    result.z = a.z * b.z;
    result.w = a.w * b.w;
    return(result);
}

internal inline f32
vec2f_dot(vec2f a, vec2f b) {
    f32 result = a.x * b.x + a.y * b.y;
    return(result);
}

internal inline f32
vec3f_dot(vec3f a, vec3f b) {
    f32 result = a.x * b.x + a.y * b.y + a.z * b.z;
    return(result);
}

internal inline f32
vec4f_dot(vec4f a, vec4f b) {
    f32 result = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    return(result);
}
