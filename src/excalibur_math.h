#ifndef EXCALIBUR_MATH_H
#define EXCALIBUR_MATH_H

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

internal inline vec2i _vec2i(s32 x, s32 y);
internal inline vec2i _vec2i(s32 s);

internal inline vec2f _vec2f(f32 x, f32 y);
internal inline vec2f _vec2f(f32 f);

internal inline vec3f _vec3f(f32 x, f32 y, f32 z);
internal inline vec3f _vec3f(vec2f v, f32 z);
internal inline vec3f _vec3f(f32 f);

internal inline vec4f _vec4f(f32 x, f32 y, f32 z, f32 w);
internal inline vec4f _vec4f(vec2f v, f32 z, f32 w);
internal inline vec4f _vec4f(vec3f v, f32 w);
internal inline vec4f _vec4f(f32 f);

internal inline vec2i operator+(vec2i a, vec2i b);
internal inline vec2f operator+(vec2f a, vec2f b);
internal inline vec3f operator+(vec3f a, vec3f b);
internal inline vec4f operator+(vec4f a, vec4f b);

internal inline vec2i &operator+=(vec2i &a, vec2i b);
internal inline vec2f &operator+=(vec2f &a, vec2f b);
internal inline vec3f &operator+=(vec3f &a, vec3f b);
internal inline vec4f &operator+=(vec4f &a, vec4f b);

internal inline vec2i operator-(vec2i v);
internal inline vec2f operator-(vec2f v);
internal inline vec3f operator-(vec3f v);
internal inline vec4f operator-(vec4f v);

internal inline vec2i operator-(vec2i a, vec2i b);
internal inline vec2f operator-(vec2f a, vec2f b);
internal inline vec3f operator-(vec3f a, vec3f b);
internal inline vec4f operator-(vec4f a, vec4f b);

internal inline vec2i &operator-=(vec2i &a, vec2i b);
internal inline vec2f &operator-=(vec2f &a, vec2f b);
internal inline vec3f &operator-=(vec3f &a, vec3f b);
internal inline vec4f &operator-=(vec4f &a, vec4f b);

internal inline vec2i operator*(s32 s, vec2i v);
internal inline vec2f operator*(f32 s, vec2f v);
internal inline vec3f operator*(f32 s, vec3f v);
internal inline vec4f operator*(f32 s, vec4f v);

internal inline vec2i operator*(vec2i v, s32 s);
internal inline vec2f operator*(vec2f v, f32 s);
internal inline vec3f operator*(vec3f v, f32 s);
internal inline vec4f operator*(vec4f v, f32 s);

internal inline vec2i &operator*=(vec2i &v, s32 s);
internal inline vec2f &operator*=(vec2f &v, f32 s);
internal inline vec3f &operator*=(vec3f &v, f32 s);
internal inline vec4f &operator*=(vec4f &v, f32 s);

internal inline vec2i operator/(vec2i v, s32 s);
internal inline vec2f operator/(vec2f v, f32 s);
internal inline vec3f operator/(vec3f v, f32 s);
internal inline vec4f operator/(vec4f v, f32 s);

internal inline vec2f vec_hadamard(vec2f a, vec2f b);
internal inline vec3f vec_hadamard(vec3f a, vec3f b);
internal inline vec4f vec_hadamard(vec4f a, vec4f b);

internal inline f32 vec_dot(vec2f a, vec2f b);
internal inline f32 vec_dot(vec3f a, vec3f b);
internal inline f32 vec_dot(vec4f a, vec4f b);

internal inline f32 vec_length_sqr(vec2f v);
internal inline f32 vec_length(vec2f v);

internal inline f32 sqr(f32 f);

struct rect2f {
    vec2f min;
    vec2f max;
};

internal inline rect2f rect2f_min_max(vec2f min, vec2f max);
internal inline rect2f rect2f_min_dim(vec2f min, vec2f dim);
internal inline rect2f rect2f_center_half_dim(vec2f center, vec2f half_dim);
internal inline rect2f rect2f_center_dim(vec2f center, vec2f dim);

internal inline b32 is_in_rect(rect2f rect, vec2f test);

internal inline vec2f rect_get_min_corner(rect2f rect);
internal inline vec2f rect_get_max_corner(rect2f rect);
internal inline vec2f rect_get_center(rect2f rect);

#endif // EXCALIBUR_MATH_H
