SHELL = /bin/bash
CC = gcc

all: server
server: server.c
	${CC} server.c -o server

exe:
	./server

clean:
	rm -f server *.o