#ifndef EXCALIBUR_CONTEXT_H
#define EXCALIBUR_CONTEXT_H

////////////////////////////////
// NOTE(xkazu0x): Clang OS/Arch cracking

#if defined(__clang__)
# define COMPILER_CLANG 1

# if defined(_WIN32)
#  define OS_WINDOWS 1
# elif defined(__gnu_linux__) || defined(__linux__)
#  define OS_LINUX 1
# elif defined(__APPLE__) && defined(__MACH__)
#  define OS_MAC 1
# else
#  error Compiler/OS is not supported.
# endif

# if defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || defined(__x86_64)
#  define ARCH_X64 1
# elif defined(i386) || defined(__i386) || defined(__i386__)
#  define ARCH_X86 1
# elif defined(__aarch64__)
#  define ARCH_ARM64 1
# elif defined(__arm__)
#  define ARCH_ARM32 1
# else
#  error Architecture is not supported.
# endif

////////////////////////////////
// NOTE(xkazu0x): MSVC OS/Arch cracking

#elif defined(_MSC_VER)
# define COMPILER_MSVC 1

# if defined(_WIN32)
#  define OS_WINDOWS 1
# else
#  error Compiler/OS is not supported.
# endif

# if defined(_M_AMD64)
#  define ARCH_X64 1
# elif defined(_M_IX86)
#  define ARCH_X86 1
# elif defined(_M_ARM64)
#  define ARCH_ARM64 1
# elif defined(_M_ARM)
#  define ARCH_ARM32 1
# else
#  error Architecture not supported.
# endif

////////////////////////////////
// NOTE(xkazu0x): GCC OS/Arch cracking

#elif defined(__GNUC__)
# define COMPILER_GCC 1

# if defined(_WIN32)
#  define OS_WINDOWS 1
# elif defined(__gnu_linux__)
#  define OS_LINUX 1
# elif defined(__APPLE__) && defined(__MACH__)
#  define OS_MAC 1
# else
#  error Compiler/OS is not supported.
# endif

# if defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || defined(__x86_64)
#  define ARCH_X64 1
# elif defined(i386) || defined(__i386) || defined(__i386__)
#  define ARCH_X86 1
# elif defined(__aarch64__)
#  define ARCH_ARM64 1
# elif defined(__arm__)
#  define ARCH_ARM32 1
# else
#  error Architecture is not supported.
# endif

#else
# error Compiler is not supported.
#endif

////////////////////////////////
// NOTE(xkazu0x): Arch cracking

#if defined(ARCH_X64)
# define ARCH_64BIT 1
#elif defined(ARCH_X86)
# define ARCH_32BIT 1
#endif

#if ARCH_ARM32 || ARCH_ARM64 || ARCH_X64 || ARCH_X86
# define ARCH_LITTLE_ENDIAN 1
#else
# error Endianness of this architecture not understood by context cracker.
#endif

////////////////////////////////
// NOTE(xkazu0x): Language cracking

#if defined(__cplusplus)
# define LANG_CPP 1
#else
# define LANG_C 1
#endif

////////////////////////////////
// NOTE(xkazu0x): Build option cracking

#if !defined(EXCALIBUR_DEBUG)
# define EXCALIBUR_DEBUG 1
#endif

////////////////////////////////
// NOTE(xkazu0x): Zero all undefined options

#if !defined(ARCH_32BIT)
# define ARCH_32BIT 0
#endif
#if !defined(ARCH_64BIT)
# define ARCH_64BIT 0
#endif
#if !defined(ARCH_X64)
# define ARCH_X64 0
#endif
#if !defined(ARCH_X86)
# define ARCH_X86 0
#endif
#if !defined(ARCH_ARM64)
# define ARCH_ARM64 0
#endif
#if !defined(ARCH_ARM32)
# define ARCH_ARM32 0
#endif
#if !defined(COMPILER_MSVC)
# define COMPILER_MSVC 0
#endif
#if !defined(COMPILER_GCC)
# define COMPILER_GCC 0
#endif
#if !defined(COMPILER_CLANG)
# define COMPILER_CLANG 0
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

////////////////////////////////
// NOTE(xkazu0x): Unsupported errors

#if ARCH_X86
# error You tried to build in x86 (32 bit) mode, but currently, only building in x64 (64 bit) mode is supported.
#endif
#if !ARCH_X64
# error You tried to build with an unsupported architecture. Currently, only building in x64 mode is supported.
#endif

#endif // EXCALIBUR_CONTEXT_H
