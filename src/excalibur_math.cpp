internal inline vec2i
_vec2i(s32 x, s32 y) {
    vec2i result;
    result.x = x;
    result.y = y;
    return(result);
}

internal inline vec2i
_vec2i(s32 s) {
    vec2i result;
    result.x = s;
    result.y = s;
    return(result);
}

internal inline vec2f
_vec2f(f32 x, f32 y) {
    vec2f result;
    result.x = x;
    result.y = y;
    return(result);
}

internal inline vec2f
_vec2f(f32 f) {
    vec2f result;
    result.x = f;
    result.y = f;
    return(result);
}

internal inline vec3f
_vec3f(f32 x, f32 y, f32 z) {
    vec3f result;
    result.x = x;
    result.y = y;
    result.z = z;
    return(result);
}

internal inline vec3f
_vec3f(vec2f v, f32 z) {
    vec3f result;
    result.x = v.x;
    result.y = v.y;
    result.z = z;
    return(result);
}

internal inline vec3f
_vec3f(f32 f) {
    vec3f result;
    result.x = f;
    result.y = f;
    result.z = f;
    return(result);
}

internal inline vec4f
_vec4f(f32 x, f32 y, f32 z, f32 w) {
    vec4f result;
    result.x = x;
    result.y = y;
    result.z = z;
    result.w = w;
    return(result);
}

internal inline vec4f
_vec4f(vec2f v, f32 z, f32 w) {
    vec4f result;
    result.x = v.x;
    result.y = v.y;
    result.z = z;
    result.w = w;
    return(result);
}

internal inline vec4f
_vec4f(vec3f v, f32 w) {
    vec4f result;
    result.x = v.x;
    result.y = v.y;
    result.z = v.z;
    result.w = w;
    return(result);
}

internal inline vec4f
_vec4f(f32 f) {
    vec4f result;
    result.x = f;
    result.y = f;
    result.z = f;
    result.w = f;
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

internal inline vec2i &
operator+=(vec2i &a, vec2i b) {
    a = a + b;
    return(a);
}

internal inline vec2f &
operator+=(vec2f &a, vec2f b) {
    a = a + b;
    return(a);
}

internal inline vec3f &
operator+=(vec3f &a, vec3f b) {
    a = a + b;
    return(a);
}

internal inline vec4f &
operator+=(vec4f &a, vec4f b) {
    a = a + b;
    return(a);
}

internal inline vec2i
operator-(vec2i v) {
    vec2i result;
    result.x = -v.x;
    result.y = -v.y;
    return(result);
}

internal inline vec2f
operator-(vec2f v) {
    vec2f result;
    result.x = -v.x;
    result.y = -v.y;
    return(result);
}

internal inline vec3f
operator-(vec3f v) {
    vec3f result;
    result.x = -v.x;
    result.y = -v.y;
    result.z = -v.z;
    return(result);
}

internal inline vec4f
operator-(vec4f v) {
    vec4f result;
    result.x = -v.x;
    result.y = -v.y;
    result.z = -v.z;
    result.w = -v.w;
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

internal inline vec2i &
operator-=(vec2i &a, vec2i b) {
    a = a - b;
    return(a);
}

internal inline vec2f &
operator-=(vec2f &a, vec2f b) {
    a = a - b;
    return(a);
}

internal inline vec3f &
operator-=(vec3f &a, vec3f b) {
    a = a - b;
    return(a);
}

internal inline vec4f &
operator-=(vec4f &a, vec4f b) {
    a = a - b;
    return(a);
}

internal inline vec2i
operator*(s32 s, vec2i v) {
    vec2i result;
    result.x = s*v.x;
    result.y = s*v.y;
    return(result);
}

internal inline vec2f
operator*(f32 s, vec2f v) {
    vec2f result;
    result.x = s*v.x;
    result.y = s*v.y;
    return(result);
}

internal inline vec3f
operator*(f32 s, vec3f v) {
    vec3f result;
    result.x = s*v.x;
    result.y = s*v.y;
    result.z = s*v.z;
    return(result);
}

internal inline vec4f
operator*(f32 s, vec4f v) {
    vec4f result;
    result.x = s*v.x;
    result.y = s*v.y;
    result.z = s*v.z;
    result.w = s*v.w;
    return(result);
}

internal inline vec2i
operator*(vec2i v, s32 s) {
    vec2i result;
    result.x = v.x*s;
    result.y = v.y*s;
    return(result);
}

internal inline vec2f
operator*(vec2f v, f32 s) {
    vec2f result;
    result.x = v.x*s;
    result.y = v.y*s;
    return(result);
}

internal inline vec3f
operator*(vec3f v, f32 s) {
    vec3f result;
    result.x = v.x*s;
    result.y = v.y*s;
    result.z = v.z*s;
    return(result);
}

internal inline vec4f
operator*(vec4f v, f32 s) {
    vec4f result;
    result.x = v.x*s;
    result.y = v.y*s;
    result.z = v.z*s;
    result.w = v.w*s;
    return(result);
}

internal inline vec2i &
operator*=(vec2i &v, s32 s) {
    v = v*s;
    return(v);
}

internal inline vec2f &
operator*=(vec2f &v, f32 s) {
    v = v*s;
    return(v);
}

internal inline vec3f &
operator*=(vec3f &v, f32 s) {
    v = v*s;
    return(v);
}

internal inline vec4f &
operator*=(vec4f &v, f32 s) {
    v = v*s;
    return(v);
}

internal inline vec2i
operator/(vec2i v, s32 s) {
    vec2i result;
    result.x = v.x/s;
    result.y = v.y/s;
    return(result);
}

internal inline vec2f
operator/(vec2f v, f32 s) {
    vec2f result;
    result.x = v.x/s;
    result.y = v.y/s;
    return(result);
}

internal inline vec3f
operator/(vec3f v, f32 s) {
    vec3f result;
    result.x = v.x/s;
    result.y = v.y/s;
    result.z = v.z/s;
    return(result);
}

internal inline vec4f
operator/(vec4f v, f32 s) {
    vec4f result;
    result.x = v.x/s;
    result.y = v.y/s;
    result.z = v.z/s;
    result.w = v.w/s;
    return(result);
}

internal inline vec2f
vec_hadamard(vec2f a, vec2f b) {
    vec2f result;
    result.x = a.x*b.x;
    result.y = a.y*b.y;
    return(result);
}

internal inline vec3f
vec_hadamard(vec3f a, vec3f b) {
    vec3f result;
    result.x = a.x*b.x;
    result.y = a.y*b.y;
    result.z = a.z*b.z;
    return(result);
}

internal inline vec4f
vec_hadamard(vec4f a, vec4f b) {
    vec4f result;
    result.x = a.x*b.x;
    result.y = a.y*b.y;
    result.z = a.z*b.z;
    result.w = a.w*b.w;
    return(result);
}

internal inline f32
vec_dot(vec2f a, vec2f b) {
    f32 result = a.x*b.x + a.y*b.y;
    return(result);
}

internal inline f32
vec_dot(vec3f a, vec3f b) {
    f32 result = a.x*b.x + a.y*b.y + a.z*b.z;
    return(result);
}

internal inline f32
vec_dot(vec4f a, vec4f b) {
    f32 result = a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w;
    return(result);
}

internal inline f32
vec_length_sqr(vec2f v) {
    f32 result = vec_dot(v, v);
    return(result);
}

internal inline f32
vec_length(vec2f v) {
    f32 result = sqrt_f32(vec_length_sqr(v));
    return(result);
}

internal inline f32
sqr(f32 f) {
    f32 result = f*f;
    return(result);
}

internal inline rect2f
rect2f_min_max(vec2f min, vec2f max) {
    rect2f result;
    result.min = min;
    result.min = max;
    return(result);
}

internal inline rect2f
rect2f_min_dim(vec2f min, vec2f dim) {
    rect2f result;
    result.min = min;
    result.min = min + dim;
    return(result);
}

internal inline rect2f
rect2f_center_half_dim(vec2f center, vec2f half_dim) {
    rect2f result;
    result.min = center - half_dim;
    result.max = center + half_dim;
    return(result);
}

internal inline rect2f
rect2f_center_dim(vec2f center, vec2f dim) {
    rect2f result = rect2f_center_half_dim(center, 0.5f*dim);
    return(result);
}

internal inline b32
is_in_rect(rect2f rect, vec2f test) {
    b32 result = ((test.x >= rect.min.x) &&
                  (test.y >= rect.min.y) &&
                  (test.x < rect.max.x) &&
                  (test.y < rect.max.y));
    return(result);
}

internal inline vec2f
rect_get_min_corner(rect2f rect) {
    vec2f result = rect.min;
    return(result);
}

internal inline vec2f
rect_get_max_corner(rect2f rect) {
    vec2f result = rect.max;
    return(result);
}

internal inline vec2f
rect_get_center(rect2f rect) {
    vec2f result = 0.5f*(rect.min + rect.max);
    return(result);
}
