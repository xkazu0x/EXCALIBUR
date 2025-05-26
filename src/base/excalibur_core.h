#ifndef EXCALIBUR_CORE_H
#define EXCALIBUR_CORE_H

////////////////////////////////
// NOTE(xkazu0x): Linkage keyword macros

#if LANG_CPP
# define C_LINKAGE_BEGIN extern "C"{
# define C_LINKAGE_END }
# define C_LINKAGE extern "C"
#else
# define C_LINKAGE_BEGIN
# define C_LINKAGE_END
# define C_LINKAGE
#endif

#if OS_WINDOWS
# define shared_function C_LINKAGE __declspec(dllexport)
#else
# define shared_function C_LINKAGE
#endif

////////////////////////////////
// NOTE(xkazu0x): Helper macros

#if COMPILER_MSVC
# define trap() __debugbreak()
#elif COMPILER_CLANG || COMPILER_GCC
# define trap() __builtin_trap()
#else
# error Unknown trap intrinsic for this compiler.
#endif

#define assert_always(x) do {if (!(x)) {trap();}} while(0)
#if EXCALIBUR_DEBUG
# define assert(x) assert_always(x)
#else
# define assert(x) (void)(x)
#endif

#define invalid_path         assert(!"Invalid Path!")
#define invalid_default_case default: {invalid_path;} break;

#define KB(x) (((u64)(x)) << 10)
#define MB(x) (((u64)(x)) << 20)
#define GB(x) (((u64)(x)) << 30)
#define TB(x) (((u64)(x)) << 40)
#define thousand(x) ((x)*1000)
#define million(x)  ((x)*1000000)
#define billion(x)  ((x)*1000000000)

#define min(a, b) ((a)<(b)?(a):(b))
#define max(a, b) ((a)>(b)?(a):(b))

#define array_count(x) (sizeof(x)/sizeof((x)[0]))

#define internal static
#define global   static
#define local    static

////////////////////////////////
// NOTE(xkazu0x): Base types

#include <stdint.h>
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;
typedef s8       b8;
typedef s16      b16;
typedef s32      b32;
typedef s64      b64;
typedef float    f32;
typedef double   f64;

#include <stddef.h>
typedef size_t memi;

////////////////////////////////
// NOTE(xkazu0x): Constants

global s8  s8_min  = (s8) 0x80;
global s16 s16_min = (s16)0x8000;
global s32 s32_min = (s32)0x80000000;
global s64 s64_min = (s64)0x8000000000000000llu;

global s8  s8_max  = (s8) 0x7F;
global s16 s16_max = (s16)0x7FFF;
global s32 s32_max = (s32)0x7FFFFFFF;
global s64 s64_max = (s64)0x7FFFFFFFFFFFFFFFllu;

global u8  u8_max  = 0xFF;
global u16 u16_max = 0xFFFF;
global u32 u32_max = 0xFFFFFFFF;
global u64 u64_max = 0xFFFFFFFFFFFFFFFFllu;

#include <float.h>
global f32 f32_max = FLT_MAX;
global f32 f32_min = -FLT_MAX;

global f32 pi32 = 3.14159265359f;
global f64 pi64 = 3.14159265359;

////////////////////////////////
// NOTE(xkazu0x): Context enums

enum Operating_System {
    OperatingSystem_Null,
    OperatingSystem_Windows,
    OperatingSystem_Linux,
    OperatingSystem_Mac,
    OperatingSystem_Count,
};

enum Architecture {
    Architecture_Null,
    Architecture_x64,
    Architecture_x86,
    Architecture_arm64,
    Architecture_arm32,
    Architecture_Count,
};

enum Compiler {
    Compiler_Null,
    Compiler_msvc,
    Compiler_gcc,
    Compiler_clang,
    Compiler_Count,
};

////////////////////////////////
// NOTE(xkazu0x): Context functions

internal Operating_System operating_system_from_context(void);
internal Architecture architecture_from_context(void);
internal Compiler compiler_from_context(void);

internal char *string_from_operating_system(Operating_System os);
internal char *string_from_architecture(Architecture arch);
internal char *string_from_compiler(Compiler compiler);

#endif // EXCALIBUR_CORE_H
