#!/bin/bash

assert(){
    expected="$1"
    input="$2"

    ./9cc "$input" > tmp.s
    cd func
    g++ -c func.cpp
    cd ../
    g++ -o tmp tmp.s func/func.o -Wl,-z,noexecstack   #-Wl,-z,noexecstackはリンカオプションで警告文を消すために付けた
    ./tmp
    actual="$?"

    if [ "$actual" = "$expected" ]; then
        echo "$input => $actual"
    else
        echo "$input => $expected expected, but got $actual"
        exit 1
    fi
}

# array test
assert 0 "int main() { 
    int a[3]; 
    return 0;
}"

assert 0 "int main() { 
    int a[5][10]; 
    return 0;
}"

# sizeof test
assert 4 "int main() { 
    int x;
    return sizeof(x); 
}"

assert 8 "int main() { 
    int *x;
    return sizeof(x); 
}"

assert 4 "int main() { 
    int x;
    return sizeof(x+1); 
}"

# assert 8 "int main() { 
#     int *x;
#     return sizeof(x+2); 
# }"

assert 4 "int main() { 
    int *x;
    return sizeof(*x); 
}"

assert 4 "int main() { 
    return sizeof(1); 
}"

# pointer test
assert 3 "int main() {
    int x;
    int *y;
    y = &x;
    *y = 3;
    return x;
}"

assert 4 "int main() {
    int *p;
    alloc4(&p, 1, 2, 4, 8);
    int *q;
    q = p + 2;
    return *q;
}"

assert 2 "int main() {
    int *p;
    alloc4(&p, 1, 2, 4, 8);
    int *q;
    q = p + 3;
    q = q - 2;
    return *q;
}"

assert 12 "int main() {
    int *p;
    alloc4(&p, 1, 2, 4, 8);
    int *q;
    q = p + 2;
    int x;
    x = *q;
    q = p + 3;
    int y;
    y = *q;
    return x + y;
}"

# int test
assert 2 "int main() { int x; x = 2; return x;}"
assert 8 "int main() { 
    int a;
    int b; 
    a = 2; 
    b = 5; 
    return func(a,b, 1);
    }
int  func(int x, int y, int z){
    return x + y + z;
}"

# value test
assert 3 "int main() { return 3;}"
assert 1 "int main() return 1;"
assert 5 "
int main() {
    return func();
}
int func() {
    return 5;
}"

assert 3 "
int main() {
    return add(1,2);
}
int add(int a, int b) {
    return a+b;
}
"

assert 4 "
int main() {
    return add(1,2,3);
}
int add(int a, int b, int c) {
    return a+c;
}
"

assert 55 "
int main() {
    return sum(10);
}
int sum(int n) {
    if (n < 0) {
        return 0;
    } else {
        return n + sum(n-1);
    }
}
"
assert 55 "
int main() {
    return sum(10);
}
int sum(int n) {
    if (n < 0) return 0;
    return n + sum(n-1);
}
"

assert 55 "
int main() {
    int a;
    a = 10;
    return sum(a);
}
int sum(int n) {
    if (n < 0) {
        return 0;
    } else {
        return n + sum(n-1);
    }
}
"

assert 42 "int main() return 42;"

assert 21 "int main() return 5+20-4;"
assert 41 "int main() return 12 + 34 -  5;"
assert 45 "int main() return 3+6*7;"
assert 15 "int main() return 5*(9-6);"
assert 4 "int main() return (3+5)/2;"
assert 2 "int main() return -3 + 5;"
assert 10 "int main() return - - +10;"

# compare test
assert 0 "int main() return 0 == 1;"
assert 1 "int main() return 42==42;"
assert 1 "int main() return 0 != 1;"

assert 1 "int main() return 0<1;"
assert 0 "int main() return 0<0;"
assert 0 "int main() return 1<0;"
assert 1 "int main() return 0<=1;"
assert 1 "int main() return 0<=0;"
assert 0 "int main() return 1<=0;"

assert 0 "int main() return 0>1;"
assert 0 "int main() return 0>0;"
assert 1 "int main() return 1>0;"
assert 0 "int main() return 0>=1;"
assert 1 "int main() return 0>=0;"
assert 1 "int main() return 1>=0;"

# variable test
assert 14 "int main() {
int a;
int b;
a = 3;
b = 5 * 6 - 8;
return a + b / 2;  
} "

# return test
assert 6 "int main() {
    int foo;
    foo = 1;
    int bar; 
    bar = 2 + 3; 
    return foo+bar; 
}"
assert 5 "int main() { return 5; return 8;}"

# if test
assert 3 "int main() { int a; a = 3; if(a == 3) return a; return 5;}"
assert 5 "int main() { int a; a = 3; if(a != 3) return a; return 5;}"
assert 5 "int main() { int a; a = 3; if(a != 3) return a; else return 5;}"

# while test
assert 11 "int main() { 
int i;
i=0; 
while(i<=10) i=i+1; 
return i;
}"

# for test
assert 30 "int main() { int a; int i; a=0; for(i=0; i<10; i=i+1) a=a+2; return i+a;}"
assert 10 "int main() { int a; int i; a=0; for(;a<10;) a=a+1; return a;}"
# assert 0 "int main() return for(;;) 10; return 5;"  segmentation fault

# mult test
assert 6 "int main() {
int a; 
a = 3;
if(a == 1) return 4;
if(a == 2) return 5;
if(a == 3) return 6;
}"

# block test
assert 10 "int main() { 
int a;
a = 0;
for(;;) {
    a = a + 1;
    if(a==5) return 10;
}
return 2;
}"

# function test
assert 1 "int main() { return foo(); }"
assert 7 "int main() { return bar(3,4); }"
assert 35 "int main() { return piyo(5, 9, 21); }"

# address, dereference test
assert 3 "int main() {
    int x;
    int y;
    x = 3;
    y = &x;
    return *y;
}"

assert 3 "int main() {
    int x;
    int y;
    int z;
    x = 3;
    y = 5;
    z = &y + 8;
    return *z;
}"

echo OK