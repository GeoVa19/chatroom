CC=gcc
CFLAGS=-O2

server: server.o
	$(CC) -o server server.o $(CFLAGS)

client: client.o
	$(CC) -o client client.o $(CFLAGS)

.PHONY: all, clean

all: server client

clean:
	rm -f *.o server client
