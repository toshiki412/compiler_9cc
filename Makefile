CXXFLAGS=-std=c++11 -g -static

9cc: 9cc.cpp

test: 9cc
	./test.sh

clean:
	rm -f 9cc *.o *~ tmp*

.PHONY: test clean
