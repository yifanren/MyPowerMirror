OBJ= server.o main.o
PROGRAM= server
OBJ=  HTTPServer.o main.o Client.o ByteBuffer.o HTTPRequest.o HTTPMessage.o  HTTPResponse.o Resource.o  ResourceHost.o
CFLAGS=-std=c++11
LIBS=
CC=g++

all:$(PROGRAM)

$(PROGRAM):$(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)
%.o:%.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	-rm -f $(OBJ) $(PROGRAM)
