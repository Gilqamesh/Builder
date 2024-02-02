# Builder

Easy to use lightweight build system.

## Prerequisites
* C compiler supporting C99 standard

## Use
Example usage on the provided examples with GNU gcc and g++:

### First example:
```
gcc -o build_simple -I. builder.c examples/simple/build_example.c
./build_simple
./examples/simple/example
```

### Second example:
```
gcc -o build_multiple_compilers -I. builder.c examples/multiple_compilers/build_example.c
./build_multiple_compilers
./examples/multiple_compilers/example
```

### Third example:
```
gcc -o build_shared_lib -I. builder.c examples/shared_lib/build_example.c
./build_shared_lib
```
Then in a separate terminal
```
cd examples/shared_lib && LD_LIBRARY_PATH=. ./example
```
Changes the files `example.c, shared_lib.c, shared_lib.h` are rebuilt and reloaded automatically.

## Todo
* Win32 API support
