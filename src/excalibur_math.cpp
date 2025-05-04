internal inline vec2
make_vec2(f32 f) {
    vec2 result;
    result.x = f;
    result.y = f;
    return(result);
}

internal inline vec3
make_vec3(f32 f) {
    vec3 result;
    result.x = f;
    result.y = f;
    result.z = f;
    return(result);
}

internal inline vec4
make_vec4(f32 f) {
    vec4 result;
    result.x = f;
    result.y = f;
    result.z = f;
    result.w = f;
    return(result);
}

internal inline vec2
make_vec2(f32 x, f32 y) {
    vec2 result;
    result.x = x;
    result.y = y;
    return(result);
}

internal inline vec3
make_vec3(f32 x, f32 y, f32 z) {
    vec3 result;
    result.x = x;
    result.y = y;
    result.z = z;
    return(result);
}

internal inline vec4
make_vec4(f32 x, f32 y, f32 z, f32 w) {
    vec4 result;
    result.x = x;
    result.y = y;
    result.z = z;
    result.w = w;
    return(result);
}

internal inline vec3
make_vec3(vec2 xy, f32 z) {
    vec3 result;
    result.x = xy.x;
    result.y = xy.y;
    result.z = z;
    return(result);
}

internal inline vec4
make_vec4(vec2 xy, f32 z, f32 w) {
    vec4 result;
    result.x = xy.x;
    result.y = xy.y;
    result.z = z;
    result.w = w;
    return(result);
}

internal inline vec4
make_vec4(vec3 xyz, f32 w) {
    vec4 result;
    result.x = xyz.x;
    result.y = xyz.y;
    result.z = xyz.z;
    result.w = w;
    return(result);
}

internal inline vec2
operator+(vec2 a, vec2 b) {
    vec2 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    return(result);
}

internal inline vec3
operator+(vec3 a, vec3 b) {
    vec3 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    return(result);
}

internal inline vec4
operator+(vec4 a, vec4 b) {
    vec4 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    result.w = a.w + b.w;
    return(result);
}

internal inline vec2 &
operator+=(vec2 &a, vec2 b) {
    a = a + b;
    return(a);
}

internal inline vec3 &
operator+=(vec3 &a, vec3 b) {
    a = a + b;
    return(a);
}

internal inline vec4 &
operator+=(vec4 &a, vec4 b) {
    a = a + b;
    return(a);
}

internal inline vec2
operator-(vec2 v) {
    vec2 result;
    result.x = -v.x;
    result.y = -v.y;
    return(result);
}

internal inline vec3
operator-(vec3 v) {
    vec3 result;
    result.x = -v.x;
    result.y = -v.y;
    result.z = -v.z;
    return(result);
}

internal inline vec4
operator-(vec4 v) {
    vec4 result;
    result.x = -v.x;
    result.y = -v.y;
    result.z = -v.z;
    result.w = -v.w;
    return(result);
}

internal inline vec2
operator-(vec2 a, vec2 b) {
    vec2 result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    return(result);
}

internal inline vec3
operator-(vec3 a, vec3 b) {
    vec3 result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;
    return(result);
}

internal inline vec4
operator-(vec4 a, vec4 b) {
    vec4 result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;
    result.w = a.w - b.w;
    return(result);
}

internal inline vec2 &
operator-=(vec2 &a, vec2 b) {
    a = a - b;
    return(a);
}

internal inline vec3 &
operator-=(vec3 &a, vec3 b) {
    a = a - b;
    return(a);
}

internal inline vec4 &
operator-=(vec4 &a, vec4 b) {
    a = a - b;
    return(a);
}

internal inline vec2
operator*(f32 s, vec2 v) {
    vec2 result;
    result.x = s*v.x;
    result.y = s*v.y;
    return(result);
}

internal inline vec3
operator*(f32 s, vec3 v) {
    vec3 result;
    result.x = s*v.x;
    result.y = s*v.y;
    result.z = s*v.z;
    return(result);
}

internal inline vec4
operator*(f32 s, vec4 v) {
    vec4 result;
    result.x = s*v.x;
    result.y = s*v.y;
    result.z = s*v.z;
    result.w = s*v.w;
    return(result);
}

internal inline vec2
operator*(vec2 v, f32 s) {
    vec2 result;
    result.x = v.x*s;
    result.y = v.y*s;
    return(result);
}

internal inline vec3
operator*(vec3 v, f32 s) {
    vec3 result;
    result.x = v.x*s;
    result.y = v.y*s;
    result.z = v.z*s;
    return(result);
}

internal inline vec4
operator*(vec4 v, f32 s) {
    vec4 result;
    result.x = v.x*s;
    result.y = v.y*s;
    result.z = v.z*s;
    result.w = v.w*s;
    return(result);
}

internal inline vec2 &
operator*=(vec2 &v, f32 s) {
    v = v*s;
    return(v);
}

internal inline vec3 &
operator*=(vec3 &v, f32 s) {
    v = v*s;
    return(v);
}

internal inline vec4 &
operator*=(vec4 &v, f32 s) {
    v = v*s;
    return(v);
}

internal inline b32
operator==(vec2 a, vec2 b) {
    b32 result = ((a.x == b.x) &&
                  (a.y == b.y));
    return(result);
}

internal inline b32
operator==(vec3 a, vec3 b) {
    b32 result = ((a.x == b.x) &&
                  (a.y == b.y) &&
                  (a.z == b.z));
    return(result);
}

internal inline b32
operator==(vec4 a, vec4 b) {
    b32 result = ((a.x == b.x) &&
                  (a.y == b.y) &&
                  (a.z == b.z) &&
                  (a.w == b.w));
    return(result);
}

internal inline b32
operator!=(vec2 a, vec2 b) {
    b32 result = ((a.x != b.x) &&
                  (a.y != b.y));
    return(result);
}

internal inline b32
operator!=(vec3 a, vec3 b) {
    b32 result = ((a.x != b.x) &&
                  (a.y != b.y) &&
                  (a.z != b.z));
    return(result);
}

internal inline b32
operator!=(vec4 a, vec4 b) {
    b32 result = ((a.x != b.x) &&
                  (a.y != b.y) &&
                  (a.z != b.z) &&
                  (a.w != b.w));
    return(result);
}

internal inline vec2
vec_hadamard(vec2 a, vec2 b) {
    vec2 result;
    result.x = a.x*b.x;
    result.y = a.y*b.y;
    return(result);
}

internal inline vec3
vec_hadamard(vec3 a, vec3 b) {
    vec3 result;
    result.x = a.x*b.x;
    result.y = a.y*b.y;
    result.z = a.z*b.z;
    return(result);
}

internal inline vec4
vec_hadamard(vec4 a, vec4 b) {
    vec4 result;
    result.x = a.x*b.x;
    result.y = a.y*b.y;
    result.z = a.z*b.z;
    result.w = a.w*b.w;
    return(result);
}

internal inline f32
vec_dot(vec2 a, vec2 b) {
    f32 result = a.x*b.x + a.y*b.y;
    return(result);
}

internal inline f32
vec_dot(vec3 a, vec3 b) {
    f32 result = a.x*b.x + a.y*b.y + a.z*b.z;
    return(result);
}

internal inline f32
vec_dot(vec4 a, vec4 b) {
    f32 result = a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w;
    return(result);
}

internal inline f32
vec_length_square(vec2 v) {
    f32 result = vec_dot(v, v);
    return(result);
}

internal inline f32
vec_length_square(vec3 v) {
    f32 result = vec_dot(v, v);
    return(result);
}

internal inline f32
vec_length_square(vec4 v) {
    f32 result = vec_dot(v, v);
    return(result);
}

internal inline f32
vec_length(vec2 v) {
    f32 result = square_root(vec_length_square(v));
    return(result);
}

internal inline f32
vec_length(vec3 v) {
    f32 result = square_root(vec_length_square(v));
    return(result);
}

internal inline f32
vec_length(vec4 v) {
    f32 result = square_root(vec_length_square(v));
    return(result);
}

internal inline rect2
make_rect2_min_max(vec2 min, vec2 max) {
    rect2 result;
    result.min = min;
    result.min = max;
    return(result);
}

internal inline rect3
make_rect3_min_max(vec3 min, vec3 max) {
    rect3 result;
    result.min = min;
    result.min = max;
    return(result);
}

internal inline rect2
make_rect2_min_dim(vec2 min, vec2 dim) {
    rect2 result;
    result.min = min;
    result.min = min + dim;
    return(result);
}

internal inline rect3
make_rect3_min_dim(vec3 min, vec3 dim) {
    rect3 result;
    result.min = min;
    result.min = min + dim;
    return(result);
}

internal inline rect2
make_rect2_center_half_dim(vec2 center, vec2 half_dim) {
    rect2 result;
    result.min = center - half_dim;
    result.max = center + half_dim;
    return(result);
}

internal inline rect3
make_rect3_center_half_dim(vec3 center, vec3 half_dim) {
    rect3 result;
    result.min = center - half_dim;
    result.max = center + half_dim;
    return(result);
}

internal inline rect2
make_rect2_center_dim(vec2 center, vec2 dim) {
    rect2 result = make_rect2_center_half_dim(center, 0.5f*dim);
    return(result);
}

internal inline rect3
make_rect3_center_dim(vec3 center, vec3 dim) {
    rect3 result = make_rect3_center_half_dim(center, 0.5f*dim);
    return(result);
}

internal inline vec2
get_rect_min(rect2 rect) {
    vec2 result = rect.min;
    return(result);
}

internal inline vec3
get_rect_min(rect3 rect) {
    vec3 result = rect.min;
    return(result);
}

internal inline vec2
get_rect_max(rect2 rect) {
    vec2 result = rect.max;
    return(result);
}

internal inline vec3
get_rect_max(rect3 rect) {
    vec3 result = rect.max;
    return(result);
}

internal inline vec2
get_rect_center(rect2 rect) {
    vec2 result = 0.5f*(rect.min + rect.max);
    return(result);
}

internal inline vec3
get_rect_center(rect3 rect) {
    vec3 result = 0.5f*(rect.min + rect.max);
    return(result);
}

internal inline rect2
rect_add_radius(rect2 rect, vec2 radius) {
    rect2 result;
    result.min = rect.min - radius;
    result.max = rect.max + radius;
    return(result);
}

internal inline rect3
rect_add_radius(rect3 rect, vec3 radius) {
    rect3 result;
    result.min = rect.min - radius;
    result.max = rect.max + radius;
    return(result);
}

internal inline b32
is_in_rect(rect2 rect, vec2 test) {
    b32 result = ((test.x >= rect.min.x) &&
                  (test.y >= rect.min.y) &&
                  (test.x < rect.max.x) &&
                  (test.y < rect.max.y));
    return(result);
}

internal inline b32
is_in_rect(rect3 rect, vec3 test) {
    b32 result = ((test.x >= rect.min.x) &&
                  (test.y >= rect.min.y) &&
                  (test.z >= rect.min.z) &&
                  (test.x < rect.max.x) &&
                  (test.y < rect.max.y) &&
                  (test.z < rect.max.z));
    return(result);
}

internal inline b32
rect_intersect(rect3 a, rect3 b) {
    b32 result = !((b.max.x <= a.min.x) ||
                   (b.min.x >= a.max.x) ||
                   (b.max.y <= a.min.y) ||
                   (b.min.y >= a.max.y) ||
                   (b.max.z <= a.min.z) ||
                   (b.min.z >= a.max.z));
    return(result);
}

////////////////

internal inline f32
square(f32 x) {
    f32 result = x*x;
    return(result);
}

internal inline f32
lerp(f32 a, f32 t, f32 b) {
    f32 result = (1.0f - t)*a + t*b;
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

internal inline vec2
clamp01(vec2 value) {
    vec2 result;
    result.x = clamp01(value.x);
    result.y = clamp01(value.y);
    return(result);
}

internal inline vec3
clamp01(vec3 value) {
    vec3 result;
    result.x = clamp01(value.x);
    result.y = clamp01(value.y);
    result.z = clamp01(value.z);
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

internal inline vec2
get_barycentric(rect2 rect, vec2 point) {
    vec2 result;
    result.x = safe_ratio0(point.x - rect.min.x, rect.max.x - rect.min.x);
    result.y = safe_ratio0(point.y - rect.min.y, rect.max.y - rect.min.y);
    return(result);
}

internal inline vec3
get_barycentric(rect3 rect, vec3 point) {
    vec3 result;
    result.x = safe_ratio0(point.x - rect.min.x, rect.max.x - rect.min.x);
    result.y = safe_ratio0(point.y - rect.min.y, rect.max.y - rect.min.y);
    result.z = safe_ratio0(point.z - rect.min.z, rect.max.z - rect.min.z);
    return(result);
}

internal inline vec3
make_rgb(f32 r, f32 g, f32 b) {
    vec3 result;
    result.r = r/255.0f;
    result.g = g/255.0f;
    result.b = b/255.0f;
    return(result);
}

internal inline vec4
make_rgba(f32 r, f32 g, f32 b, f32 a) {
    vec4 result;
    result.r = r/255.0f;
    result.g = g/255.0f;
    result.b = b/255.0f;
    result.a = a/255.0f;
    return(result);
}
