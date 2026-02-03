# Define the files to format (C source and headers in the current directory)
SOURCES := $(wildcard *.c *.h)

all: toyforth

# generate format file with:
# clang-format -style=llvm -dump-config > .clang-format
.PHONY: format
format:
	clang-format -i $(SOURCES)

toyforth: main.c
	# build with warnings and optimization level 2
	gcc -Wall -W -O2 main.c -o toyforth

# debug build with symbols for valgrind/gdb
debug: main.c
	gcc -Wall -W -g -O0 main.c -o toyforth

# run valgrind memory leak check (requires debug build)
.PHONY: valgrind
valgrind: debug
	valgrind --leak-check=full --show-leak-kinds=all ./toyforth nums.ft

clean:
	rm -f toyforth

