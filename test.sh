
runTest() {
    ./9cc "test/test.cpp" > tmp.s
    cd func
    g++ -c func.cpp
    cd ../
    g++ -static -o tmp tmp.s func/func.o -Wl,-z,noexecstack   #-Wl,-z,noexecstack�̓����J�I�v�V�����Ōx�������������߂ɕt����
    ./tmp
}

runTest