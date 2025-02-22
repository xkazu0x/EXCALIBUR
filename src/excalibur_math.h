#ifndef EXCALIBUR_MATH_H
#define EXCALIBUR_MATH_H

////////////////////////////////
// NOTE(xkazu0x): compound types

union vec2i {
    struct {
        s32 x, y;
    };
    struct {
        s32 u, v;
    };
    s32 e[2];
};

union vec2f {
    struct {
        f32 x, y;
    };
    struct {
        f32 u, v;
    };
    f32 e[2];
};

union vec3f {
    struct {
        f32 x, y, z;
    };
    struct {
        f32 u, v, w;
    };
    struct {
        f32 r, g, b;
    };
    f32 e[3];
};

union vec4f {
    struct {
        f32 x, y, z, w;
    };
    struct {
        f32 r, g, b, a;
    };
    f32 e[4];
};

////////////////////////////////
// NOTE(xkazu0x): math functions

internal f32 abs_f32(f32 x);
internal f64 abs_f64(f64 x);

internal f32 sqrt_f32(f32 x);
internal f32 sin_f32(f32 x);
internal f32 cos_f32(f32 x);
internal f32 tan_f32(f32 x);

internal f64 sqrt_f64(f64 x);
internal f64 sin_f64(f64 x);
internal f64 cos_f64(f64 x);
internal f64 tan_f64(f64 x);

/////////////////////////////////////////
// NOTE(xkazu0x): compound type functions

internal inline vec2i vec2i_create(s32 x, s32 y);
internal inline vec2f vec2f_create(f32 x, f32 y);
internal inline vec3f vec3f_create(f32 x, f32 y, f32 z);
internal inline vec4f vec4f_create(f32 x, f32 y, f32 z, f32 w);

internal inline vec2i operator+(vec2i a, vec2i b);
internal inline vec2f operator+(vec2f a, vec2f b);
internal inline vec3f operator+(vec3f a, vec3f b);
internal inline vec4f operator+(vec4f a, vec4f b);

internal inline vec2i operator-(vec2i a, vec2i b);
internal inline vec2f operator-(vec2f a, vec2f b);
internal inline vec3f operator-(vec3f a, vec3f b);
internal inline vec4f operator-(vec4f a, vec4f b);

internal inline vec2i operator*(vec2i v, s32 s);
internal inline vec2f operator*(vec2f v, f32 s);
internal inline vec3f operator*(vec3f v, f32 s);
internal inline vec4f operator*(vec4f v, f32 s);

internal inline vec2i operator*(s32 s, vec2i v);
internal inline vec2f operator*(f32 s, vec2f v);
internal inline vec3f operator*(f32 s, vec3f v);
internal inline vec4f operator*(f32 s, vec4f v);

internal inline vec2i operator/(vec2i v, s32 s);
internal inline vec2f operator/(vec2f v, f32 s);
internal inline vec3f operator/(vec3f v, f32 s);
internal inline vec4f operator/(vec4f v, f32 s);

internal inline vec2f vec2f_hadamard(vec2f a, vec2f b);
internal inline vec3f vec3f_hadamard(vec3f a, vec3f b);
internal inline vec4f vec4f_hadamard(vec4f a, vec4f b);

internal inline f32 vec2f_dot(vec2f a, vec2f b);
internal inline f32 vec3f_dot(vec3f a, vec3f b);
internal inline f32 vec4f_dot(vec4f a, vec4f b);

#endif // EXCALIBUR_MATH_H
