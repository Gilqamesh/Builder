# Builder

Lightweight, easy-to-use live framework language for managing dependencies and executing build processes continuously to keep your programs up-to-date.

## Prerequisites
* C compiler supporting C99 standard

## Use
Example usage on the provided examples with GNU gcc and g++:

```
gcc -o build_examples build_examples.c builder.c
./build_examples
```

## Features
* Specify a list of dependencies: `obj__dependencies(dep1, dep2, ...)`
* File object: `obj__file(path)`
* Process object: `obj__process(opt_deps_in, opt_deps_out, cmd_line)`
> resulting flow graph: optional in dependency object(s) -> cmd_line to execute in bash -> optional out dependency object(s)
* Build: `obj__build(obj)`
> ensures that the specified object is up-to-date
* Print: `obj__print(obj)`
> prints obj graph, example:
```
/usr/bin/sh -c "./example"
  example
    /usr/bin/sh -c "/usr/bin/g++ lib/lib.o example.o example.cpp.o -o example | head -n 25"
      example.o
        /usr/bin/sh -c "/usr/bin/gcc -Ilib -g -c example.c -o example.o -Wall -Wextra -Werror 2>&1 | head -n 25"
          /usr/bin/gcc
          example.h
          example.c
      example.cpp.o
        /usr/bin/sh -c "/usr/bin/g++ -Ilib -g -c example.cpp -o example.cpp.o -Wall -Wextra -Werror 2>&1 | head -n 25"
          /usr/bin/g++
          lib/lib.h
          example.cpp
          example.h
      lib/lib.o
        /usr/bin/sh -c "/usr/bin/gcc -g -c lib/lib.c -o lib/lib.o -Wall -Wextra -Werror 2>&1 | head -n 25"
          /usr/bin/gcc
          lib/lib.h
          lib/lib.c
      /usr/bin/g++
```

## Todo
* Windows support
* Make more object types
