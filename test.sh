
runTest() {
    ./9cc "test/test.cpp" > tmp.s
    cd func
    g++ -c func.cpp
    cd ../
    g++ -static -o tmp tmp.s func/func.o -Wl,-z,noexecstack   #-Wl,-z,noexecstackはリンカオプションで警告文を消すために付けた
    ./tmp
}

runTest