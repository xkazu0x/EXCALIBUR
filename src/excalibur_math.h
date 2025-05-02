#ifndef EXCALIBUR_MATH_H
#define EXCALIBUR_MATH_H

union vec2 {
    struct {
        f32 x, y;
    };
    struct {
        f32 u, v;
    };
    f32 e[2];
};

union vec3 {
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
        vec2 xy;
        f32 ignored0_;
    };
    f32 e[3];
};

union vec4 {
    struct {
        f32 x, y, z, w;
    };
    struct {
        f32 r, g, b, a;
    };
    f32 e[4];
};

internal inline vec2 make_vec2(f32 f);
internal inline vec3 make_vec3(f32 f);
internal inline vec4 make_vec4(f32 f);

internal inline vec2 make_vec2(f32 x, f32 y);
internal inline vec3 make_vec3(f32 x, f32 y, f32 z);
internal inline vec4 make_vec4(f32 x, f32 y, f32 z, f32 w);

internal inline vec3 make_vec3(vec2 xy, f32 z);
internal inline vec4 make_vec4(vec2 xy, f32 z, f32 w);
internal inline vec4 make_vec4(vec3 xyz, f32 w);

internal inline vec2 operator+(vec2 a, vec2 b);
internal inline vec3 operator+(vec3 a, vec3 b);
internal inline vec4 operator+(vec4 a, vec4 b);

internal inline vec2 &operator+=(vec2 &a, vec2 b);
internal inline vec3 &operator+=(vec3 &a, vec3 b);
internal inline vec4 &operator+=(vec4 &a, vec4 b);

internal inline vec2 operator-(vec2 v);
internal inline vec3 operator-(vec3 v);
internal inline vec4 operator-(vec4 v);

internal inline vec2 operator-(vec2 a, vec2 b);
internal inline vec3 operator-(vec3 a, vec3 b);
internal inline vec4 operator-(vec4 a, vec4 b);

internal inline vec2 &operator-=(vec2 &a, vec2 b);
internal inline vec3 &operator-=(vec3 &a, vec3 b);
internal inline vec4 &operator-=(vec4 &a, vec4 b);

internal inline vec2 operator*(f32 s, vec2 v);
internal inline vec3 operator*(f32 s, vec3 v);
internal inline vec4 operator*(f32 s, vec4 v);

internal inline vec2 operator*(vec2 v, f32 s);
internal inline vec3 operator*(vec3 v, f32 s);
internal inline vec4 operator*(vec4 v, f32 s);

internal inline vec2 &operator*=(vec2 &v, f32 s);
internal inline vec3 &operator*=(vec3 &v, f32 s);
internal inline vec4 &operator*=(vec4 &v, f32 s);

internal inline b32 operator==(vec2 a, vec2 b);
internal inline b32 operator==(vec3 a, vec3 b);
internal inline b32 operator==(vec4 a, vec4 b);

internal inline b32 operator!=(vec2 a, vec2 b);
internal inline b32 operator!=(vec3 a, vec3 b);
internal inline b32 operator!=(vec4 a, vec4 b);

internal inline vec2 vec_hadamard(vec2 a, vec2 b);
internal inline vec3 vec_hadamard(vec3 a, vec3 b);
internal inline vec4 vec_hadamard(vec4 a, vec4 b);

internal inline f32 vec_dot(vec2 a, vec2 b);
internal inline f32 vec_dot(vec3 a, vec3 b);
internal inline f32 vec_dot(vec4 a, vec4 b);

internal inline f32 vec_length_square(vec2 v);
internal inline f32 vec_length_square(vec3 v);
internal inline f32 vec_length_square(vec4 v);

internal inline f32 vec_length(vec2 v);
internal inline f32 vec_length(vec3 v);
internal inline f32 vec_length(vec4 v);

struct rect2 {
    vec2 min;
    vec2 max;
};

struct rect3 {
    vec3 min;
    vec3 max;
};

internal inline rect2 make_rect2_min_max(vec2 min, vec2 max);
internal inline rect3 make_rect3_min_max(vec3 min, vec3 max);

internal inline rect2 make_rect2_min_dim(vec2 min, vec2 dim);
internal inline rect3 make_rect3_min_dim(vec3 min, vec3 dim);

internal inline rect2 make_rect2_center_half_dim(vec2 center, vec2 half_dim);
internal inline rect3 make_rect3_center_half_dim(vec3 center, vec3 half_dim);

internal inline rect2 make_rect2_center_dim(vec2 center, vec2 dim);
internal inline rect3 make_rect3_center_dim(vec3 center, vec3 dim);

internal inline vec2 get_rect_min(rect2 rect);
internal inline vec3 get_rect_min(rect3 rect);

internal inline vec2 get_rect_max(rect2 rect);
internal inline vec3 get_rect_max(rect3 rect);

internal inline vec2 get_rect_center(rect2 rect);
internal inline vec3 get_rect_center(rect3 rect);

internal inline rect2 rect_add_radius(rect2 rect, vec2 radiuse);
internal inline rect3 rect_add_radius(rect3 rect, vec3 radius);

internal inline b32 is_in_rect(rect2 rect, vec2 test);
internal inline b32 is_in_rect(rect3 rect, vec3 test);

internal inline b32 rect_intersect(rect3 a, rect3 b);

//
//
//

internal inline f32 square(f32 x);

internal inline vec3 make_rgb(f32 r, f32 g, f32 b);
internal inline vec4 make_rgba(f32 r, f32 g, f32 b, f32 a);

#endif // EXCALIBUR_MATH_H
