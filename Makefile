CC = gcc
CFLAGS = -Wall -Werror -pedantic -O2 -g
STD = c99
TEST_EXE = ./vmap_test

.PHONY: all
all: libvmap.a

.PHONY: test
test: vmap_test
	$(TEST_EXE)

vmap_test: test.c vmap.h libvmap.a
	$(CC) $(CFLAGS) -o $(TEST_EXE) test.c -L. libvmap.a

vmap.o: vmap.c
	$(CC) $(CFLAGS) -std=$(STD) -c -o $@ $<

libvmap.a: vmap.o
	ar rcs $@ $<

.PHONY: clean
clean:
	rm -f vmap.o libvmap.a $(TEST_EXE)
