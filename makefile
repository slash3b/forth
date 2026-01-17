# Define the files to format (C source and headers in the current directory)
SOURCES := $(wildcard *.c *.h)

all: toyforth

# generate format file with:
# clang-format -style=llvm -dump-config > .clang-format
.PHONY: format
format:
	clang-format -i $(SOURCES)

toyforth: main.c
	# build with warnings and optimization livel 2
	gcc -Wall -W 02 main.c -o toyforth

clean:
	rm toyforth

