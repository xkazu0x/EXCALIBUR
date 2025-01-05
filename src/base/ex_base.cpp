#include "ex_base.h"

function OPERATING_SYSTEM
operating_system_from_context(void) {
    OPERATING_SYSTEM result = OPERATING_SYSTEM_UNDEFINED;
#if OS_WINDOWS
    result = OPERATING_SYSTEM_WINDOWS;
#elif OS_LINUX
    result = OPERATING_SYSTEM_LINUX;
#elif OS_MAC
    result = OPERATING_SYSTEM_MAC;
#endif
    return(result);
}

function ARCHITECTURE
architecture_from_context(void) {
    ARCHITECTURE result = ARCHITECTURE_UNDEFINED;
#if ARCH_X64
    result = ARCHITECTURE_X64;
#elif ARCH_X86
    result = ARCHITECTURE_X86;
#elif ARCH_ARM
    result = ARCHITECTURE_ARM;
#elif ARCH_ARM64
    result = ARCHITECTURE_ARM64;
#endif
    return(result);
}

function const char *
string_from_operating_system(OPERATING_SYSTEM os) {
    const char *result = "";
    switch(os) {
        case OPERATING_SYSTEM_WINDOWS: {
            result = "WINDOWS";
        } break;
        case OPERATING_SYSTEM_LINUX: {
            result = "LINUX";
        } break;
        case OPERATING_SYSTEM_MAC: {
            result = "MAC";
        } break;
        default: {
            result = "UNDEFINED";
        }
    }
    return(result);
}

function const char *
string_from_architecture(ARCHITECTURE arch) {
    const char *result = "";
    switch(arch) {
        case ARCHITECTURE_X64: {
            result = "X64";
        } break;
        case ARCHITECTURE_X86: {
            result = "X86";
        } break;
        case ARCHITECTURE_ARM: {
            result = "ARM";
        } break;
        case ARCHITECTURE_ARM64: {
            result = "ARM64";
        } break;
        default: {
            result = "UNDEFINED";
        }
    }
    return(result);
}

////////////////////////////////
// NOTE(xkazu0x): math functions

function float32
abs_f32(float32 x) {
    union { float32 f; uint32 u; } result;
    result.f = x;
    result.u &= 0x7fffffff;
    return(result.f);
}

function float64
abs_f64(float64 x) {
    union { float64 f; uint64 u; } result;
    result.f = x;
    result.u &= 0x7fffffffffffffff;
    return(result.f);
}

#include <math.h>

function float32
sqrt_f32(float32 x) {
    return(sqrtf(x));
}

function float32
sin_f32(float32 x) {
    return(sinf(x));
}

function float32
cos_f32(float32 x) {
    return(cosf(x));
}

function float32
tan_f32(float32 x) {
    return(tanf(x));
}

function float64
sqrt_f64(float64 x) {
    return(sqrtf(x));
}

function float64
sin_f64(float64 x) {
    return(sinf(x));
}

function float64
cos_f64(float64 x) {
    return(cosf(x));
}

function float64
tan_f64(float64 x) {
    return(tanf(x));
}

function inline vec2i
vec2i_create(int32 x, int32 y) {
    vec2i result = {x, y};
    return(result);
}

function inline vec2f
vec2f_create(float32 x, float32 y) {
    vec2f result = {x, y};
    return(result);
}

function inline vec3f
vec3f_create(float32 x, float32 y, float32 z) {
    vec3f result = {x, y, z};
    return(result);
}

function inline vec4f
vec4f_create(float32 x, float32 y, float32 z, float32 w) {
    vec4f result = {x, y, z, w};
    return(result);
}

function inline vec2i
operator+(const vec2i &a, const vec2i &b) {
    vec2i result = {a.x + b.x, a.y + b.y};
    return(result);
}

function inline vec2f
operator+(const vec2f &a, const vec2f &b) {
    vec2f result = {a.x + b.x, a.y + b.y};
    return(result);
}

function inline vec3f
operator+(const vec3f &a, const vec3f &b) {
    vec3f result = {a.x + b.x, a.y + b.y, a.z + b.z};
    return(result);
}

function inline vec4f
operator+(const vec4f &a, const vec4f &b) {
    vec4f result = {a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w};
    return(result);
}

function inline vec2i
operator-(const vec2i &a, const vec2i &b) {
    vec2i result = {a.x - b.x, a.y - b.y};
    return(result);
}

function inline vec2f
operator-(const vec2f &a, const vec2f &b) {
    vec2f result = {a.x - b.x, a.y - b.y};
    return(result);
}

function inline vec3f
operator-(const vec3f &a, const vec3f &b) {
    vec3f result = {a.x - b.x, a.y - b.y, a.z - b.z};
    return(result);
}

function inline vec4f
operator-(const vec4f &a, const vec4f &b) {
    vec4f result = {a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w};
    return(result);
}

function inline vec2i
operator*(const vec2i &v, const int32 &s) {
    vec2i result = { v.x * s, v.y * s};
    return(result);
}

function inline vec2f
operator*(const vec2f &v, const float32 &s) {
    vec2f result = { v.x * s, v.y * s };
    return(result);
}

function inline vec3f
operator*(const vec3f &v, const float32 &s) {
    vec3f result = { v.x * s, v.y * s, v.z * s };
    return(result);
}

function inline vec4f
operator*(const vec4f &v, const float32 &s) {
    vec4f result = { v.x * s, v.y * s, v.z * s, v.w * s };
    return(result);
}

function inline vec2i
operator*(const int32 &s, const vec2i &v) {
    vec2i result = { s * v.x, s * v.y };
    return(result);
}

function inline vec2f
operator*(const float32 &s, const vec2f &v) {
    vec2f result = { s * v.x, s * v.y };
    return(result);
}

function inline vec3f
operator*(const float32 &s, const vec3f &v) {
    vec3f result = { s * v.x, s * v.y, s * v.z };
    return(result);
}

function inline vec4f
operator*(const float32 &s, const vec4f &v) {
    vec4f result = { s * v.x, s * v.y, s * v.z, s * v.w};
    return(result);
}

function inline vec2f
vec2_hadamard(vec2f a, vec2f b) {
    vec2f result = { a.x * b.x, a.y * b.y };
    return(result);
}

function inline vec3f
vec3_hadamard(vec3f a, vec3f b) {
    vec3f result = { a.x * b.x, a.y * b.y, a.z * b.z };
    return(result);
}

function inline vec4f
vec4_hadamard(vec4f a, vec4f b) {
    vec4f result = { a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w };
    return(result);
}

function inline float32
vec2_dot(vec2f a, vec2f b) {
    float32 result = a.x * b.x + a.y * b.y;
    return(result);
}

function inline float32
vec3_dot(vec3f a, vec3f b) {
    float32 result = a.x * b.x + a.y * b.y + a.z * b.z;
    return(result);
}

function inline float32
vec4_dot(vec4f a, vec4f b) {
    float32 result = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    return(result);
}

/////////////////////////////////
// NOTE(xkazu0x): logger function

#include <stdio.h>
#include <stdarg.h>
function void
_logger_output(EX_LOG_LEVEL level, const char *message, ...) {
    const char *levels[EX_LOG_LEVEL_MAX] {
        "[FATAL]: ",
        "[ERROR]: ",
        "[WARN]: ",
        "[INFO]: ",
        "[DEBUG]: ",
        "[TRACE]: ",
    };

    local char buffer[1024];
    va_list arg_list;
    va_start(arg_list, message);
    vsprintf_s(buffer, sizeof(buffer), message, arg_list);
    va_end(arg_list);

    local char buffer1[1024];
    sprintf_s(buffer1, "%s%s\n", levels[level], buffer);
    printf(buffer1);
}

