#include <stdio.h>

void foo () asm ("foo");

void foo()  {
    printf("function OK!\n");
}

