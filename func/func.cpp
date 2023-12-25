#include <stdio.h>
#include <cstdlib>

extern "C" void alloc4(int **p, int a, int b, int c, int d);

void alloc4(int **p, int a, int b, int c, int d) {
    *p = static_cast<int*>(std::malloc(sizeof(int) * 4));
    (*p)[0] = a;
    (*p)[1] = b;
    (*p)[2] = c;
    (*p)[3] = d;
}