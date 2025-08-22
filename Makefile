CC=gcc
CFLAGS=-Wall -Wextra -g
LFLAGS=-lm

.PHONY: run-test
run-test: build/test
	@./build/test

build/test: src/*.h test/main.c
	@mkdir -p build
	@$(CC) $(CFLAGS) $(LFLAGS) -obuild/test test/main.c

.PHONY: clean
clean:
	rm -rf build
