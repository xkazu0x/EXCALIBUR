#include "ex_base.h"
#include <stdio.h>

struct TEST_STRUCT {
    int a;
    int b;
    int c;
    int d;
};

#define EX_PRINT(x) printf("%s = %lld\n", #x, (x))

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

    EX_PRINT(EXM_OFFSETOF(TEST_STRUCT, a));
    EX_PRINT(EXM_OFFSETOF(TEST_STRUCT, b));
    EX_PRINT(EXM_OFFSETOF(TEST_STRUCT, c));
    EX_PRINT(EXM_OFFSETOF(TEST_STRUCT, d));

    TEST_STRUCT t = { 1, 2, 3, 4 };
    EX_PRINT(t.a);
    EX_PRINT(t.b);
    
    return(0);
}
