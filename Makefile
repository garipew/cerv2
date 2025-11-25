CC=gcc
CFLAGS=-Wall -Wextra -pedantic -std=c99 -g -I/usr/local/include/snorkel -L/usr/local/lib
CLIBS=-lsnorkel

cerver: main.c cerver.o
	$(CC) $(CFLAGS) -o cerver main.c cerver.o $(CLIBS)

cerver.o: cerver.c cerver.h
	$(CC) $(CFLAGS) -c cerver.c $(CLIBS)

clean:
	rm -rf *.o cerver
