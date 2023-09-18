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

# int test
assert 2 "main() { int x; x = 2; return x;}"

# value test
assert 3 "main() { return 3;}"
assert 1 "main() return 1;"
assert 5 "
main() {
    return func();
}
func() {
    return 5;
}"

assert 3 "
main() {
    return add(1,2);
}
add(a, b) {
    return a+b;
}
"

assert 4 "
main() {
    return add(1,2,3);
}
add(a, b, c) {
    return a+c;
}
"

assert 55 "
main() {
    return sum(10);
}
sum(n) {
    if (n < 0) {
        return 0;
    } else {
        return n + sum(n-1);
    }
}
"
assert 55 "
main() {
    return sum(10);
}
sum(n) {
    if (n < 0) return 0;
    return n + sum(n-1);
}
"

assert 55 "
main() {
    a = 10;
    return sum(a);
}
sum(n) {
    if (n < 0) {
        return 0;
    } else {
        return n + sum(n-1);
    }
}
"

assert 42 "main() return 42;"

assert 21 "main() return 5+20-4;"
assert 41 "main() return 12 + 34 -  5;"
assert 45 "main() return 3+6*7;"
assert 15 "main() return 5*(9-6);"
assert 4 "main() return (3+5)/2;"
assert 2 "main() return -3 + 5;"
assert 10 "main() return - - +10;"

# compare test
assert 0 "main() return 0 == 1;"
assert 1 "main() return 42==42;"
assert 1 "main() return 0 != 1;"

assert 1 "main() return 0<1;"
assert 0 "main() return 0<0;"
assert 0 "main() return 1<0;"
assert 1 "main() return 0<=1;"
assert 1 "main() return 0<=0;"
assert 0 "main() return 1<=0;"

assert 0 "main() return 0>1;"
assert 0 "main() return 0>0;"
assert 1 "main() return 1>0;"
assert 0 "main() return 0>=1;"
assert 1 "main() return 0>=0;"
assert 1 "main() return 1>=0;"

# variable test
assert 14 "main() {
a = 3;
b = 5 * 6 - 8;
return a + b / 2;  
} "

# return test
assert 6 "main() { foo = 1; bar = 2 + 3; return foo+bar; }"
assert 5 "main() { return 5; return 8;}"

# if test
assert 3 "main() { a = 3; if(a == 3) return a; return 5;}"
assert 5 "main() { a = 3; if(a != 3) return a; return 5;}"
assert 5 "main() { a = 3; if(a != 3) return a; else return 5;}"

# while test
assert 11 "main() { 
i=0; 
while(i<=10) i=i+1; 
return i;
}"

# for test
assert 30 "main() { a=0; for(i=0; i<10; i=i+1) a=a+2; return i+a;}"
assert 10 "main() { a=0; for(;a<10;) a=a+1; return a;}"
# assert 0 "main() return for(;;) 10; return 5;"  segmentation fault

# mult test
assert 6 "main() { 
a = 3;
if(a == 1) return 4;
if(a == 2) return 5;
if(a == 3) return 6;
}"

# block test
assert 10 "main() { 
a = 0;
for(;;) {
    a = a + 1;
    if(a==5) return 10;
}
return 2;
}"

# function test
assert 1 "main() { return foo(); }"
assert 7 "main() { return bar(3,4); }"
assert 35 "main() { return piyo(5, 9, 21); }"

# address, dereference test
assert 3 "main() {
    x = 3;
    y = &x;
    return *y;
}"

assert 3 "main() {
    x = 3;
    y = 5;
    z = &y + 8;
    return *z;
}"

echo OK