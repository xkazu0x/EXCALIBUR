#ifndef EX_BASE_H
#define EX_BASE_H

//////////////////////////////////
// NOTE(xkazu0x): context cracking

#if defined(__clang__)
# define COMPILER_CLANG 1

# if defined(_WIN32)
#  define OS_WINDOWS 1
# elif defined(__gnu_linux__)
#  define OS_LINUX 1
# elif defined(__APPLE__) && defined(__MACH__)
#  define OS_MAC 1
# else
#  error MISSING OPERATING SYSTEM DETECTION
# endif

# if defined(__amd64__)
#  define ARCH_X64 1
// TODO(xkazu0x): verify this works on clang
# elif defined(__i386__)
#  define ARCH_X86 1
// TODO(xkazu0x): verify this works on clang
# elif defined(__arm__)
#  define ARCH_ARM 1
// TODO(xkazu0x): verify this works on clang
# elif defined(__aarch64__)
#  define ARCH_ARM64 1
# else
#  error MISSING ARCHITECTURE DETECTION
# endif

#elif defined(_MSC_VER)
# define COMPILER_CL 1

# if defined(_WIN32)
#  define OS_WINDOWS 1
# else
#  error MISSING OPERATING SYSTEM DETECTION
# endif

# if defined(_M_AMD64)
#  define ARCH_X64 1
# elif defined(_M_I86)
#  define ARCH_X86 1
# elif defined(_M_ARM)
#  define ARCH_ARM 1
// TODO(xkazu0x): ARM64??
# else
#  error MISSING ARCHITECTURE DETECTION
# endif

#elif defined(__GNUC__)
# define COMPILER_GCC 1

# if defined(_WIN32)
#  define OS_WINDOWS 1
# elif defined(__gnu_linux__)
#  define OS_LINUX 1
# elif defined(__APPLE__) && defined(__MACH__)
#  define OS_MAC 1
# else
#  error MISSING OPERATING SYSTEM DETECTION
# endif

# if defined(__amd64__)
#  define ARCH_X64 1
# elif defined(__i386__)
#  define ARCH_X86 1
# elif defined(__arm__)
#  define ARCH_ARM 1
# elif defined(__aarch64__)
#  define ARCH_ARM64 1
# else
#  error MISSING ARCHITECTURE DETECTION
# endif

#else
# error NO CONTEXT CRACKING FOR THIS COMPILER
#endif

#if !defined(COMPILER_CL)
# define COMPILER_CL 0
#endif
#if !defined(COMPILER_CLANG)
# define COMPILER_CLANG 0
#endif
#if !defined(COMPILER_GCC)
# define COMPILER_GCC 0
#endif

#if !defined(OS_WINDOWS)
# define OS_WINDOWS 0
#endif
#if !defined(OS_LINUX)
# define OS_LINUX 0
#endif
#if !defined(OS_MAC)
# define OS_MAC 0
#endif

#if !defined(ARCH_X64)
# define ARCH_X64 0
#endif
#if !defined(ARCH_X86)
# define ARCH_X86 0
#endif
#if !defined(ARCH_ARM)
# define ARCH_ARM 0
#endif
#if !defined(ARCH_ARM64)
# define ARCH_ARM64 0
#endif

/////////////////////////////
// NOTE(xkazu0x): basic types

#include <stdint.h>
typedef int8_t  int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef int8  bool8;
typedef int16 bool16;
typedef int32 bool32;
typedef int64 bool64;
typedef float  float32;
typedef double float64;

///////////////////////////////
// NOTE(xkazu0x): helper macros

#if !defined(EX_ENABLE_ASSERT)
# define EX_ENABLE_ASSERT
#endif

#define EXM_STMNT(x) do { x } while(0)
#define EXM_ASSERT_BREAK() (*(int*)0 = 0)

#ifdef EX_ENABLE_ASSERT
# define EXM_ASSERT(x) EXM_STMNT(if (!(x)) { EXM_ASSERT_BREAK(); })
#else
# define EXM_ASSERT(x)
#endif

#define EXM_ARRAY_COUNT(x) (sizeof(x)/sizeof(*(x)))

#define EXM_INT_FROM_PTR(p) (unsigned long long)((char*)p - (char*)0)
#define EXM_PTR_FROM_INT(n) (void*)((char*)0 + (n))

#define EXM_MEMBER(T, m) (((T*)0)->m)
#define EXM_OFFSETOF(T, m) EXM_INT_FROM_PTR(&EXM_MEMBER(T, m))

#define EXM_MIN(a, b) (((a)<(b))?(a):(b))
#define EXM_MAX(a, b) (((a)>(b))?(a):(b))
#define EXM_CLAMP(a, x, b) (((x)<(a))?(a):\
                            ((b)<(x))?(b):(x))
#define EXM_CLAMP_TOP(a, b) EXM_MIN(a, b)
#define EXM_CLAMP_BOT(a, b) EXM_MAX(a, b)

#define global static
#define local static
#define function static

#define C_LINKAGE_BEGIN extern "C" {
#define C_LINKAGE_END }
#define C_LINKAGE extern "C"

#include <string.h>
#define EXM_MEMORY_ZERO(p,z) memset((p), 0, (z))
#define EXM_MEMORY_ZERO_STRUCT(p) EXM_MEMORY_ZERO((p), sizeof(*(p)))
#define EXM_MEMORY_ZERO_ARRAY(p) EXM_MEMORY_ZERO((p), sizeof(p))
#define EXM_MEMORY_ZERO_TYPED(p,c) EXM_MEMORY_ZERO((p), sizeof(*(p))*(c))

#define EXM_MEMORY_MATCH(a,b,z) (memcmp((a),(b),(z)) == 0)

#define EXM_MEMORY_COPY(d,s,z) memmove((d),(s),(z))
#define EXM_MEMORY_COPY_STRUCT(d,s) memmove((d),(s),\
                                            EXM_MIN(sizeof(*(d)), sizeof(*(s))))
#define EXM_MEMORY_COPY_ARRAY(d,s) memmove((d),(s),EXM_MIN(sizeof(s), sizeof(d)))
#define EXM_MEMORY_COPY_TYPED(d,s,c) memmove((d),(s),\
                                             EXM_MIN(sizeof(*(d)), sizeof(*(s)))*(c))

/////////////////////////////////
// NOTE(xkazu0x): basic constants

global int8  min_int8  = (int8) 0x80;
global int16 min_int16 = (int16)0x8000;
global int32 min_int32 = (int32)0x80000000;
global int64 min_int64 = (int64)0x8000000000000000llu;

global int8  max_int8  = (int8) 0x7f;
global int16 max_int16 = (int16)0x7fff;
global int32 max_int32 = (int32)0x7fffffff;
global int64 max_int64 = (int64)0x7fffffffffffffffllu;

global uint8  max_uint8  = 0xff;
global uint16 max_uint16 = 0xffff;
global uint32 max_uint32 = 0xffffffff;
global uint64 max_uint64 = 0xffffffffffffffffllu;

global float32 pi_f32 = 3.14159265359f;
global float64 pi_f64 = 3.14159265359;

////////////////////////////////
// NOTE(xkazu0x): compound types

union VEC2I {
    struct {
        int32 x;
        int32 y;
    };
    int32 data[2];
};

union VEC2F {
    struct {
        float32 x;
        float32 y;
    };
    float32 data[2];
};

union VEC3F {
    struct {
        float32 x;
        float32 y;
        float32 z;
    };
    float32 data[3];
};

union VEC4F {
    struct {
        float32 x;
        float32 y;
        float32 z;
        float32 w;
    };
    float32 data[4];
};

////////////////////////////////
// NOTE(xkazu0x): math functions

function float32 abs_f32(float32 x);
function float64 abs_f64(float64 x);

function float32 sqrt_f32(float32 x);
function float32 sin_f32(float32 x);
function float32 cos_f32(float32 x);
function float32 tan_f32(float32 x);

function float64 sqrt_f64(float64 x);
function float64 sin_f64(float64 x);
function float64 cos_f64(float64 x);
function float64 tan_f64(float64 x);

/////////////////////////////////////////
// NOTE(xkazu0x): compound type functions

function VEC2I vec2i(int32 x, int32 y);

function VEC2F vec2f(float32 x, float32 y);
function VEC3F vec3f(float32 x, float32 y, float32 z);
function VEC4F vec4f(float32 x, float32 y, float32 z, float32 w);

function VEC2I operator+(const VEC2I &a, const VEC2I &b);
function VEC2F operator+(const VEC2F &a, const VEC2F &b);
function VEC3F operator+(const VEC3F &a, const VEC3F &b);
function VEC4F operator+(const VEC4F &a, const VEC4F &b);

function VEC2I operator-(const VEC2I &a, const VEC2I &b);
function VEC2F operator-(const VEC2F &a, const VEC2F &b);
function VEC3F operator-(const VEC3F &a, const VEC3F &b);
function VEC4F operator-(const VEC4F &a, const VEC4F &b);

function VEC2I operator*(const VEC2I &v, const int32 &s);
function VEC2F operator*(const VEC2F &v, const float32 &s);
function VEC3F operator*(const VEC3F &v, const float32 &s);
function VEC4F operator*(const VEC4F &v, const float32 &s);

function VEC2I operator*(const int32 &s, const VEC2I &v);
function VEC2F operator*(const float32 &s, const VEC2F &v);
function VEC3F operator*(const float32 &s, const VEC3F &v);
function VEC4F operator*(const float32 &s, const VEC4F &v);

function VEC2F vec2_hadamard(VEC2F a, VEC2F b);
function VEC3F vec3_hadamard(VEC3F a, VEC3F b);
function VEC4F vec4_hadamard(VEC4F a, VEC4F b);

function float32 vec2_dot(VEC2F a, VEC2F b);
function float32 vec3_dot(VEC3F a, VEC3F b);
function float32 vec4_dot(VEC4F a, VEC4F b);

#endif // EX_BASE_H
