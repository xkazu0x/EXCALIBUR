#include "ex_base.h"
#include "ex_base.cpp"
#include <stdio.h>

#define EX_PRINT(x) printf("%s = %d\n", #x, (x))
#define EX_PRINTF(x) printf("%s = %f\n", #x, (float32)(x))

int main(void) {
    printf("cl      = %d\n", COMPILER_CL);
    printf("clang   = %d\n", COMPILER_CLANG);
    printf("gcc     = %d\n", COMPILER_GCC);
    printf("windows = %d\n", OS_WINDOWS);
    printf("linux   = %d\n", OS_LINUX);
    printf("mac     = %d\n", OS_MAC);
    printf("x64     = %d\n", ARCH_X64);
    printf("x86     = %d\n", ARCH_X86);
    printf("arm     = %d\n", ARCH_ARM);
    printf("arm64   = %d\n", ARCH_ARM64);

    VEC2I v0 = vec2i(20, 30);
    VEC3F v1 = vec3f(0.0f, 0.0f, 0.0f);
    VEC3F v2 = { 1.0f, 1.0f, 1.0f};

    EX_PRINT(v0.x);
    EX_PRINTF(v1.x);
    EX_PRINTF(v2.x);
    
    return(0);
}
