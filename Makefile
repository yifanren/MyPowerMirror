OBJS     = server.o client.o util.o http.o
CC      :=
LIBS    := -L./lib -lpthread -Wl,-rpath=./lib -lcurl
CFLAGS  :=
PROGRAM := server client

all: $(PROGRAM)

server : server.o util.o http.o
	cc -g -o $@ $^ $(LIBS)

client : client.o util.o
	cc -g -o $@ $^ $(LIBS)

SERVER.C = util.c http.c server.c
server.o : $(SERVER.C)
	cc -c $^

client.o : client.c util.c
	cc -c $^

clean :
	-rm -f $(OBJS) $(PROGRAM)
