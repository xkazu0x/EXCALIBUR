/////////////////////////////////////////////
// NOTE(xkazu0x): symbolic constant functions

internal OPERATING_SYSTEM
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

internal ARCHITECTURE
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

internal char *
string_from_operating_system(OPERATING_SYSTEM os) {
    char *result;
    switch(os) {
        case OPERATING_SYSTEM_WINDOWS: {
            result = "windows";
        } break;
        case OPERATING_SYSTEM_LINUX: {
            result = "linux";
        } break;
        case OPERATING_SYSTEM_MAC: {
            result = "mac";
        } break;
        default: {
            result = "undefined";
        }
    }
    return(result);
}

internal char *
string_from_architecture(ARCHITECTURE arch) {
    char *result;
    switch(arch) {
        case ARCHITECTURE_X64: {
            result = "x64";
        } break;
        case ARCHITECTURE_X86: {
            result = "x86";
        } break;
        case ARCHITECTURE_ARM: {
            result = "arm";
        } break;
        case ARCHITECTURE_ARM64: {
            result = "arm64";
        } break;
        default: {
            result = "undefined";
        }
    }
    return(result);
}

////////////////////////////////
// NOTE(xkazu0x): safe functions

internal inline u32
safe_truncate_u64(u64 value) {
    EX_ASSERT(value <= u32_max);
    u32 result = (u32)value;
    return(result);
}

internal inline s32
round_f32_to_s32(f32 f) {
    s32 result = (s32)(f + 0.5f);
    return(result);
}

internal inline u32
round_f32_to_u32(f32 f) {
    u32 result = (u32)(f + 0.5f);
    return(result);
}

// TODO(xkazu0x): implement math functions
// this shit do not fucking work
#include <math.h>
internal inline s32
floor_f32_to_s32(f32 f) {
    s32 result = (s32)floorf(f);
    return(result);
}

internal inline s32
truncate_f32_to_s32(f32 f) {
    s32 result = (s32)f;
    return(result);
}

internal inline u32
truncate_f32_to_u32(f32 f) {
    u32 result = (u32)f;
    return(result);
}

///////////////////////////////////////
// NOTE(xkazu0x): temp string functions

internal u32
string_length(char *string) {
    u32 count = 0;
    while (*string++) {
        ++count;
    }
    return(count);
}

internal void
cat_strings(size_t src_a_count, char *src_a,
            size_t src_b_count, char *src_b,
            size_t dest_count, char *dest) {
    for (u32 i = 0; i < src_a_count; ++i) {
        *dest++ = *src_a++;
    }
    for (u32 i = 0; i < src_b_count; ++i) {
        *dest++ = *src_b++;
    }
    *dest++ = 0;
}

