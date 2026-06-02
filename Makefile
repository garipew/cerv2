CC=gcc
CFLAGS=-Wall -Wextra -pedantic -std=c99 -Ivendor

cerver: src/main.c out/cerver.o out/client.o
	@mkdir -p out/bin
	$(CC) $(CFLAGS) $^ -o out/bin/$@

out/%.o: src/%.c
	@mkdir -p out
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf out/
