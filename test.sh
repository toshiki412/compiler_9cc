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

# assert 55 "main() {
#     return fractal(10);
# }
# fractal(n) {
#     if (n < 0) return 0;
#     return n + fractal(n-1);
# }
# "

# assert 42 "42;"

# assert 21 "5+20-4;"
# assert 41 "12 + 34 -  5;"
# assert 45 "3+6*7;"
# assert 15 "5*(9-6);"
# assert 4 "(3+5)/2;"
# assert 2 "-3 + 5;"
# assert 10 "- - +10;"

# # compare test
# assert 0 "0 == 1;"
# assert 1 "42==42;"
# assert 1 "0 != 1;"

# assert 1 "0<1;"
# assert 0 "0<0;"
# assert 0 "1<0;"
# assert 1 "0<=1;"
# assert 1 "0<=0;"
# assert 0 "1<=0;"

# assert 0 "0>1;"
# assert 0 "0>0;"
# assert 1 "1>0;"
# assert 0 "0>=1;"
# assert 1 "0>=0;"
# assert 1 "1>=0;"

# # variable test
# assert 14 "a = 3;
# b = 5 * 6 - 8;
# a + b / 2;"

# # return test
# assert 6 "foo = 1; bar = 2 + 3; return foo+bar;"
# assert 5 "return 5; return 8;"

# # if test
# assert 3 "a = 3; if(a == 3) return a; return 5;"
# assert 5 "a = 3; if(a != 3) return a; return 5;"
# assert 5 "a = 3; if(a != 3) return a; else return 5;"

# # while test
# assert 11 "i=0; 
# while(i<=10) i=i+1; 
# return i;"

# # for test
# assert 30 "a=0; for(i=0; i<10; i=i+1) a=a+2; return i+a;"
# assert 10 "a=0; for(;a<10;) a=a+1; return a;"
# # assert 0 "for(;;) 10; return 5;"  segmentation fault

# # mult test
# assert 6 "a = 3;
# if(a == 1) return 4;
# if(a == 2) return 5;
# if(a == 3) return 6;"

# # block test
# assert 10 "a = 0;
# for(;;) {
#     a = a + 1;
#     if(a==5) return 10;
# }
# return 2;"

# # function test
# assert 0 "foo();"
# assert 0 "bar(3,4);"
# assert 0 "piyo(5, 9, 21);"

echo OK