OBJ= server.o main.o
PROGRAM= server
CFLAGS=-std=c++11
LIBS=
CC=g++

all: $(PROGRAM)

server : $(OBJ)
	CC -o $@ $^ $(CFLAGS)

$(OBJ) : 

g++ -o server HTTPServer.cpp main.cpp Client.cpp ByteBuffer.cpp HTTPRequest.cpp HTTPMessage.cpp  HTTPResponse.cpp Resource.cpp  ResourceHost.cpp -std=c++11
g++ -o server HTTPServer.cpp main.cpp Client.cpp -std=c++11
