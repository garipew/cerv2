CC=gcc
CFLAGS=-Wall -Wextra -pedantic -std=c99 -g

all: cerver.o arena.o cerver

arena.o: arena.c arena.h
	$(CC) $(CFLAGS) -c arena.c

cerver.o: cerver.c cerver.h
	$(CC) $(CFLAGS) -c cerver.c

cerver: main.c cerver.o arena.o
	$(CC) $(CFLAGS) -o cerver main.c cerver.o arena.o

clean:
	rm -rf *.o cerver
