# Builder

Easy to use lightweight cross-platform build system.

## Prerequisites
* C compiler supporting C99 standard

## Use
Example usege on the provided examples with GNU gcc and g++:

- `gcc -o build_simple -I. builder.c examples/simple/build_example.c`
- `./build_simple`
- `./examples/simple/example`

- `gcc -o build_multiple_compilers -I. builder.c examples/multiple_compilers/build_example.c`
- `./build_multiple_compilers`
- `./examples/multiple_compilers/example`

- `gcc -o build_shared_lib -I. builder.c examples/shared_lib/build_example.c`
- in first terminal `./build_shared_lib`
- in second terminal: `cd examples/shared_lib && LD_LIBRARY_PATH=. ./example`, make some changes in example.c, shared_lib.c, or shared_lib.h

## Todo
* Win32 API support
