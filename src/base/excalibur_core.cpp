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

internal char *
string_from_operating_system(Operating_System os) {
    switch (os) {
        case OperatingSystem_Windows: return("windows");
        case OperatingSystem_Linux:   return("linux");
        case OperatingSystem_Mac:     return("mac");
        default:                      return("<null>");
    }
}

internal char *
string_from_architecture(Architecture arch) {
    switch (arch) {
        case Architecture_x64:   return("x64");
        case Architecture_x86:   return("x86");
        case Architecture_arm64: return("arm64");
        case Architecture_arm32: return("arm32");
        default:                 return("<null>");
    }
}

internal char *
string_from_compiler(Compiler compiler) {
    switch (compiler) {
        case Compiler_msvc:  return("msvc");
        case Compiler_gcc:   return("gcc");
        case Compiler_clang: return("clang");
        default:             return("<null>");
    }
}
