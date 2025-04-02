#include <math.h>

internal s32
round_f32_to_s32(f32 f) {
    s32 result = (s32)roundf(f);
    return(result);
}

internal u32
round_f32_to_u32(f32 f) {
    u32 result = (u32)roundf(f);
    return(result);
}

internal s32
truncate_f32_to_s32(f32 f) {
    s32 result = (s32)f;
    return(result);
}

internal u32
truncate_f32_to_u32(f32 f) {
    u32 result = (u32)f;
    return(result);
}

internal s32
floor_f32_to_s32(f32 f) {
    s32 result = (s32)floorf(f);
    return(result);
}

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
