#include <stdio.h>

// void foo() asm("foo");
extern "C" void foo();
extern "C" void bar(int x, int y);
extern "C" void piyo(int x, int y, int z);

void foo()  {
    printf("function OK!\n");
}

void bar(int x, int y){
    printf("%d\n",x+y);
}

void piyo(int x, int y, int z){
    printf("%d + %d + %d = %d\n", x, y, z, x+y+z);
}
