#include <stdio.h>

// void foo() asm("foo");
extern "C" int foo();
extern "C" int bar(int x, int y);
extern "C" int piyo(int x, int y, int z);

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
