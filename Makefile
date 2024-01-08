CXX=g++ #C++�R���p�C�����w�肷��
CXXFLAGS=-std=c++20 -g -static
LDFLAGS=-lstdc++ #�K�v�ȃ��C�u������ǉ�����
SRCS=$(wildcard *.cpp)
OBJS=$(SRCS:.cpp=.o)

# make�R�}���h�̈������Ȃ��ꍇ�́A�ŏ��̃^�[�Q�b�g�����s����̂ŁA���ꂪ���s�����
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
