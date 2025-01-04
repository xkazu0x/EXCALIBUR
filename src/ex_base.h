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

///////////////////////////////
// NOTE(xkazu0x): helper macros

#if !defined(EXM_ENABLE_ASSERT)
# define EXM_ENABLE_ASSERT
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

#endif // EX_BASE_H
