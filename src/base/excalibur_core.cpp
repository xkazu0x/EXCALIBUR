////////////////////////////////
// NOTE(xkazu0x): Context functions

internal Operating_System
operating_system_from_context(void) {
    Operating_System result = OperatingSystem_Null;
#if OS_WINDOWS
    result = OperatingSystem_Windows;
#elif OS_LINUX
    result = OperatingSystem_Linux;
#elif OS_MAC
    result = OperatingSystem_Mac;
#endif
    return(result);
}

internal Architecture
architecture_from_context(void) {
    Architecture result = Architecture_Null;
#if ARCH_X64
    result = Architecture_x64;
#elif ARCH_X86
    result = Architecture_x86;
#elif ARCH_ARM64
    result = Architecture_arm64;
#elif ARCH_ARM32
    result = Architecture_arm32;
#endif
    return(result);
}

internal Compiler
compiler_from_context(void) {
    Compiler result = Compiler_Null;
#if COMPILER_MSVC
    result = Compiler_msvc;
#elif COMPILER_GCC
    result = Compiler_gcc;
#elif COMPILER_CLANG
    result = Compiler_clang;
#endif
    return(result);
}

internal String8
string_from_operating_system(Operating_System os) {
    String8 result;
    switch (os) {
        case OperatingSystem_Windows: {
            result = string8("windows");
        } break;
        case OperatingSystem_Linux: {
            result = string8("linux");
        } break;
        case OperatingSystem_Mac: {
            result = string8("mac");
        } break;
        default: {
            result = string8("<null>");
        } break;
    }
    return(result);
}

internal String8
string_from_architecture(Architecture arch) {
    String8 result;
    switch (arch) {
        case Architecture_x64: {
            result = string8("x64");
        } break;
        case Architecture_x86: {
            result = string8("x86");
        } break;
        case Architecture_arm64: {
            result = string8("arm64");
        } break;
        case Architecture_arm32: {
            result = string8("arm32");
        } break;
        default: {
            result = string8("<null>");
        } break;
    }
    return(result);
}

internal String8
string_from_compiler(Compiler compiler) {
    String8 result;
    switch (compiler) {
        case Compiler_msvc: {
            result = string8("msvc");
        } break;
        case Compiler_gcc: {
            result = string8("gcc");
        } break;
        case Compiler_clang: {
            result = string8("clang");
        } break;
        default: {
            result = string8("<null>");
        } break;
    }
    return(result);
}
