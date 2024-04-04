CC = gcc
CFLAGS = -Wall -Werror -pedantic -O2 -g
STD = c99
TEST_EXE = ./vmap_test
BENCH4_EXE = ./vmap_bench4
BENCH64_EXE = ./vmap_bench64

.PHONY: all
all: libvmap.a

.PHONY: test
test: vmap_test
	$(TEST_EXE)

.PHONY: bench
bench: bench4 bench64

.PHONY: bench4
bench4: vmap_bench4
	./random_kvs.py 4 100 | $(BENCH4_EXE)

.PHONY: bench64
bench64: vmap_bench64
	./random_kvs.py 64 100 | $(BENCH64_EXE)

.PHONY: util
util:
	$(MAKE) -C util

vmap_test: test.c vmap.h libvmap.a
	$(CC) $(CFLAGS) -o $(TEST_EXE) test.c -L. libvmap.a

vmap_bench4: bench.c vmap.h libvmap.a util
	$(CC) $(CFLAGS) -DKEY_SIZE=4 -o $(BENCH4_EXE) bench.c util/util.o -L. libvmap.a

vmap_bench64: bench.c vmap.h libvmap.a util
	$(CC) $(CFLAGS) -DKEY_SIZE=64 -o $(BENCH64_EXE) bench.c util/util.o -L. libvmap.a

vmap.o: vmap.c
	$(CC) $(CFLAGS) -std=$(STD) -c -o $@ $<

libvmap.a: vmap.o
	ar rcs $@ $<

.PHONY: clean
clean:
	$(MAKE) clean -C util
	rm -f vmap.o libvmap.a $(TEST_EXE) $(BENCH4_EXE) $(BENCH64_EXE)
