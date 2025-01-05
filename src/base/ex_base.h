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

#define EX_STMNT(x) do { x } while(0)
#define EX_ASSERT_BREAK() (*(int*)0 = 0)

#ifdef EX_DEBUG_MODE
# define EX_ASSERT(x) EX_STMNT(if (!(x)) { EX_ASSERT_BREAK(); })
#else
# define EX_ASSERT(x)
#endif

#define EX_ARRAY_COUNT(x) (sizeof(x)/sizeof(*(x)))

#define EX_INT_FROM_PTR(p) (unsigned long long)((char*)p - (char*)0)
#define EX_PTR_FROM_INT(n) (void*)((char*)0 + (n))

#define EX_MEMBER(T, m) (((T*)0)->m)
#define EX_OFFSETOF(T, m) EX_INT_FROM_PTR(&EX_MEMBER(T, m))

#define EX_MIN(a, b) (((a)<(b))?(a):(b))
#define EX_MAX(a, b) (((a)>(b))?(a):(b))
#define EX_CLAMP(a, x, b) (((x)<(a))?(a):\
                           ((b)<(x))?(b):(x))
#define EX_CLAMP_TOP(a, b) EXM_MIN(a, b)
#define EX_CLAMP_BOT(a, b) EXM_MAX(a, b)

#define EX_FALSE 0
#define EX_TRUE 1

#define global static
#define local static
#define function static

#define C_LINKAGE_BEGIN extern "C" {
#define C_LINKAGE_END }
#define C_LINKAGE extern "C"

#include <string.h>
#define EX_MEMORY_ZERO(p,z) memset((p), 0, (z))
#define EX_MEMORY_ZERO_STRUCT(p) EX_MEMORY_ZERO((p), sizeof(*(p)))
#define EX_MEMORY_ZERO_ARRAY(p) EX_MEMORY_ZERO((p), sizeof(p))
#define EX_MEMORY_ZERO_TYPED(p,c) EX_MEMORY_ZERO((p), sizeof(*(p))*(c))

#define EX_MEMORY_MATCH(a,b,z) (memcmp((a),(b),(z)) == 0)

#define EX_MEMORY_COPY(d,s,z) memmove((d),(s),(z))
#define EX_MEMORY_COPY_STRUCT(d,s) memmove((d),(s),\
                                           EX_MIN(sizeof(*(d)), sizeof(*(s))))
#define EX_MEMORY_COPY_ARRAY(d,s) memmove((d),(s),EX_MIN(sizeof(s), sizeof(d)))
#define EX_MEMORY_COPY_TYPED(d,s,c) memmove((d),(s),\
                                            EX_MIN(sizeof(*(d)), sizeof(*(s)))*(c))

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

////////////////////////////////////
// NOTE(xkazu0x): symbolic constants

enum OPERATING_SYSTEM {
    OPERATING_SYSTEM_UNDEFINED,
    OPERATING_SYSTEM_WINDOWS,
    OPERATING_SYSTEM_LINUX,
    OPERATING_SYSTEM_MAC,
    OPERATING_SYSTEM_MAX,
};

enum ARCHITECTURE {
    ARCHITECTURE_UNDEFINED,
    ARCHITECTURE_X64,
    ARCHITECTURE_X86,
    ARCHITECTURE_ARM,
    ARCHITECTURE_ARM64,
    ARCHITECTURE_MAX,
};

////////////////////////////////
// NOTE(xkazu0x): compound types

union vec2i {
    struct {
        int32 x;
        int32 y;
    };
    int32 data[2];
};

union vec2f {
    struct {
        float32 x;
        float32 y;
    };
    float32 data[2];
};

union vec3f {
    struct {
        float32 x;
        float32 y;
        float32 z;
    };
    float32 data[3];
};

union vec4f {
    struct {
        float32 x;
        float32 y;
        float32 z;
        float32 w;
    };
    float32 data[4];
};

/////////////////////////////////////////////
// NOTE(xkazu0x): symbolic constant functions

function OPERATING_SYSTEM operating_system_from_context(void);
function ARCHITECTURE architecture_from_context(void);

function const char *string_from_operating_system(OPERATING_SYSTEM os);
function const char *string_from_architecture(ARCHITECTURE arch);

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

function inline vec2i vec2i_create(int32 x, int32 y);
function inline vec2f vec2f_create(float32 x, float32 y);
function inline vec3f vec3f_create(float32 x, float32 y, float32 z);
function inline vec4f vec4f_create(float32 x, float32 y, float32 z, float32 w);

function inline vec2i operator+(const vec2i &a, const vec2i &b);
function inline vec2f operator+(const vec2f &a, const vec2f &b);
function inline vec3f operator+(const vec3f &a, const vec3f &b);
function inline vec4f operator+(const vec4f &a, const vec4f &b);

function inline vec2i operator-(const vec2i &a, const vec2i &b);
function inline vec2f operator-(const vec2f &a, const vec2f &b);
function inline vec3f operator-(const vec3f &a, const vec3f &b);
function inline vec4f operator-(const vec4f &a, const vec4f &b);

function inline vec2i operator*(const vec2i &v, const int32 &s);
function inline vec2f operator*(const vec2f &v, const float32 &s);
function inline vec3f operator*(const vec3f &v, const float32 &s);
function inline vec4f operator*(const vec4f &v, const float32 &s);

function inline vec2i operator*(const int32 &s, const vec2i &v);
function inline vec2f operator*(const float32 &s, const vec2f &v);
function inline vec3f operator*(const float32 &s, const vec3f &v);
function inline vec4f operator*(const float32 &s, const vec4f &v);

function inline vec2f vec2_hadamard(vec2f a, vec2f b);
function inline vec3f vec3_hadamard(vec3f a, vec3f b);
function inline vec4f vec4_hadamard(vec4f a, vec4f b);

function inline float32 vec2_dot(vec2f a, vec2f b);
function inline float32 vec3_dot(vec3f a, vec3f b);
function inline float32 vec4_dot(vec4f a, vec4f b);

////////////////////////
// NOTE(xkazu0x): logger

#define EX_ENABLE_WARN 1
#define EX_ENABLE_INFO 1

#ifdef EX_DEBUG_MODE
#define EX_ENABLE_DEBUG 1
#define EX_ENABLE_TRACE 1
#endif

enum EX_LOG_LEVEL {
    EX_LOG_LEVEL_FATAL,
    EX_LOG_LEVEL_ERROR,
    EX_LOG_LEVEL_WARN,
    EX_LOG_LEVEL_INFO,
    EX_LOG_LEVEL_DEBUG,
    EX_LOG_LEVEL_TRACE,
    EX_LOG_LEVEL_MAX,
};

function void _logger_output(EX_LOG_LEVEL level, const char *message, ...);

#define EXFATAL(message, ...) _logger_output(EX_LOG_LEVEL_FATAL, message, ##__VA_ARGS__);
#define EXERROR(message, ...) _logger_output(EX_LOG_LEVEL_ERROR, message, ##__VA_ARGS__);
#if EX_ENABLE_WARN == 1
#define EXWARN(message, ...) _logger_output(EX_LOG_LEVEL_WARN, message, ##__VA_ARGS__);
#else
#define EXWARN(message, ...)
#endif
#if EX_ENABLE_INFO == 1
#define EXINFO(message, ...) _logger_output(EX_LOG_LEVEL_INFO, message, ##__VA_ARGS__);
#else
#define EXINFO(message, ...)
#endif
#if EX_ENABLE_DEBUG == 1
#define EXDEBUG(message, ...) _logger_output(EX_LOG_LEVEL_DEBUG, message, ##__VA_ARGS__);
#else
#define EXDEBUG(message, ...)
#endif
#if EX_ENABLE_TRACE == 1
#define EXTRACE(message, ...) _logger_output(EX_LOG_LEVEL_TRACE, message, ##__VA_ARGS__);
#else
#define EXTRACE(message, ...)
#endif

#endif // EX_BASE_H
