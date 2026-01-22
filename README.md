# toyforth

A toy Forth interpreter written in C, inspired by [Salvatore Sanfilippo's "Learn C" YouTube series](https://www.youtube.com/playlist?list=PL5o_UAwH0hOwOs_q2FQyhuZBHcHTHYFEB).

## Building

```bash
make
```

This compiles `main.c` with GCC and produces the `toyforth` executable.

## Usage

```bash
./toyforth <filename>
./toyforth -d <filename>  # debug mode
```

## Examples

Create a file with Forth code and run it:

```forth
# nums.ft - basic arithmetic
10 20 + 1 +
```

```bash
$ ./toyforth nums.ft
[10,20,+,1,+]
Stack context at end:
[31]
```

Currently supported operations: `+`, `-`, `*`, `/`, `%`

## Reference

To experiment with a full Forth implementation, you can run gforth via Docker:

```bash
docker run -it --rm forthy42/gforth
```

## License

Apache 2.0
