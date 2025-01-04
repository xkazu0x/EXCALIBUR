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

function VEC2I
vec2i(int32 x, int32 y) {
    VEC2I result = {x, y};
    return(result);
}

function VEC2F
vec2f(float32 x, float32 y) {
    VEC2F result = {x, y};
    return(result);
}

function VEC3F
vec3f(float32 x, float32 y, float32 z) {
    VEC3F result = {x, y, z};
    return(result);
}

function VEC4F
vec4f(float32 x, float32 y, float32 z, float32 w) {
    VEC4F result = {x, y, z, w};
    return(result);
}

function VEC2I
operator+(const VEC2I &a, const VEC2I &b) {
    VEC2I result = {a.x + b.x, a.y + b.y};
    return(result);
}

function VEC2F
operator+(const VEC2F &a, const VEC2F &b) {
    VEC2F result = {a.x + b.x, a.y + b.y};
    return(result);
}

function VEC3F
operator+(const VEC3F &a, const VEC3F &b) {
    VEC3F result = {a.x + b.x, a.y + b.y, a.z + b.z};
    return(result);
}

function VEC4F
operator+(const VEC4F &a, const VEC4F &b) {
    VEC4F result = {a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w};
    return(result);
}

function VEC2I
operator-(const VEC2I &a, const VEC2I &b) {
    VEC2I result = {a.x - b.x, a.y - b.y};
    return(result);
}

function VEC2F
operator-(const VEC2F &a, const VEC2F &b) {
    VEC2F result = {a.x - b.x, a.y - b.y};
    return(result);
}

function VEC3F
operator-(const VEC3F &a, const VEC3F &b) {
    VEC3F result = {a.x - b.x, a.y - b.y, a.z - b.z};
    return(result);
}

function VEC4F
operator-(const VEC4F &a, const VEC4F &b) {
    VEC4F result = {a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w};
    return(result);
}

function VEC2I
operator*(const VEC2I &v, const int32 &s) {
    VEC2I result = { v.x * s, v.y * s};
    return(result);
}

function VEC2F
operator*(const VEC2F &v, const float32 &s) {
    VEC2F result = { v.x * s, v.y * s };
    return(result);
}

function VEC3F
operator*(const VEC3F &v, const float32 &s) {
    VEC3F result = { v.x * s, v.y * s, v.z * s };
    return(result);
}

function VEC4F
operator*(const VEC4F &v, const float32 &s) {
    VEC4F result = { v.x * s, v.y * s, v.z * s, v.w * s };
    return(result);
}

function VEC2I
operator*(const int32 &s, const VEC2I &v) {
    VEC2I result = { s * v.x, s * v.y };
    return(result);
}

function VEC2F
operator*(const float32 &s, const VEC2F &v) {
    VEC2F result = { s * v.x, s * v.y };
    return(result);
}

function VEC3F
operator*(const float32 &s, const VEC3F &v) {
    VEC3F result = { s * v.x, s * v.y, s * v.z };
    return(result);
}

function VEC4F
operator*(const float32 &s, const VEC4F &v) {
    VEC4F result = { s * v.x, s * v.y, s * v.z, s * v.w};
    return(result);
}

function VEC2F
vec2_hadamard(VEC2F a, VEC2F b) {
    VEC2F result = { a.x * b.x, a.y * b.y };
    return(result);
}

function VEC3F
vec3_hadamard(VEC3F a, VEC3F b) {
    VEC3F result = { a.x * b.x, a.y * b.y, a.z * b.z };
    return(result);
}

function VEC4F
vec4_hadamard(VEC4F a, VEC4F b) {
    VEC4F result = { a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w };
    return(result);
}

function float32
vec2_dot(VEC2F a, VEC2F b) {
    float32 result = a.x * b.x + a.y * b.y;
    return(result);
}

function float32
vec3_dot(VEC3F a, VEC3F b) {
    float32 result = a.x * b.x + a.y * b.y + a.z * b.z;
    return(result);
}

function float32
vec4_dot(VEC4F a, VEC4F b) {
    float32 result = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    return(result);
}
