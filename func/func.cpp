#include <stdio.h>

// void foo() asm("foo");
extern "C" void foo();

void foo()  {
    printf("function OK!\n");
}

