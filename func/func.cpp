#include <stdio.h>
#include <cstdlib>

// void foo() asm("foo");
extern "C" int foo();
extern "C" int bar(int x, int y);
extern "C" int piyo(int x, int y, int z);
extern "C" void alloc4(int **p, int a, int b, int c, int d);

int foo()  {
    printf("function OK!\n");
    return 1;
}

int bar(int x, int y) {
    printf("%d\n",x+y);
    return x + y;
}


int piyo(int x, int y, int z) {
    printf("%d + %d + %d = %d\n", x, y, z, x+y+z);
    return x + y + z;
}

void alloc4(int **p, int a, int b, int c, int d) {
    *p = static_cast<int*>(std::malloc(sizeof(int) * 4));
    (*p)[0] = a;
    (*p)[1] = b;
    (*p)[2] = c;
    (*p)[3] = d;
}