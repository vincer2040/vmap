CC = gcc
CFLAGS = -Wall -Werror -pedantic -O2 -g

.PHONY: all
all: util.o

util.o: util.c util.h
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: clean
clean:
	rm -f util.o
