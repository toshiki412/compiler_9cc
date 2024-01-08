CXX=g++ #C++コンパイラを指定する
CXXFLAGS=-std=c++20 -g -static
LDFLAGS=-lstdc++ #必要なライブラリを追加する
SRCS=$(wildcard *.cpp)
OBJS=$(SRCS:.cpp=.o)

# makeコマンドの引数がない場合は、最初のターゲットを実行するので、これが実行される
9cc: $(OBJS)
		$(CXX) -o 9cc $(OBJS) $(LDFLAGS)

$(OBJS): 9cc.h

test: 9cc
	./test.sh

self: 9cc
	./9cc selfcompile.h tokenize.cpp > self.s

clean:
	rm -f 9cc *.o *~ tmp*

.PHONY: test clean self
