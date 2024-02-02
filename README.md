# Builder

Easy to use lightweight cross-platform build system.

## Prerequisites
* C compiler supporting C99 standard

## Use

Example use on the provided example with GNU gcc and g++:
- `gcc -o build_simple -I. builder.c examples/simple/build_example.c`
- `gcc -o build_multiple_compilers -I. builder.c examples/multiple_compilers/build_example.c`

## Todo
* Avoid building up-to-date files (currently, the whole input module and its dependencies are rebuilt)
