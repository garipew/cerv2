CC=gcc
CFLAGS=-Wall -Wextra -pedantic -std=c99 -g

cerver: main.c cerver.o client.o
	$(CC) $(CFLAGS) -o cerver main.c cerver.o client.o

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf *.o cerver
