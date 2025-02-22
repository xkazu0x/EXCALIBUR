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
