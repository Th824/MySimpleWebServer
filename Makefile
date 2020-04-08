# makefile
MAIN := main.cpp
SOURCE := $(wildcard *.cpp base/*.cpp)
OBJS := $(patsubst %.cpp, %.o, $(SOURCE))

TARGET := Server
CC := g++
LIBS := -lpthread
INCLUDE := -I./usr/local/lib
CFLAGS := -std=c++11 -g
CXXFLAGS := $(CFLAGS)

$(TARGET) : $(OBJS) main.o
	$(CC) $(CXXFLAGS) -o $@ $^ $(LIBS)

.PHONY : clean obj
objs : $(OBJS)
clean :
	rm *.o