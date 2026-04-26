CC=gcc
CFLAGS=-Wall -Wextra -pedantic -std=c99 -g

cerver: snorkel/snorkel_arena.h main.c cerver.o
	$(CC) $(CFLAGS) -o cerver main.c cerver.o

cerver.o: snorkel/snorkel_arena.h cerver.c cerver.h
	$(CC) $(CFLAGS) -c cerver.c

clean:
	rm -rf *.o cerver
