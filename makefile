# Define the files to format (C source and headers in the current directory)
SOURCES := $(wildcard *.c *.h)

# generate format file with:
# clang-format -style=llvm -dump-config > .clang-format

.PHONY: format
format:
	clang-format -i $(SOURCES)


