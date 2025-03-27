# Builder

## What is the vision of the project?
* Graph model of programming, where Nodes are programs with states (objects), and Directed Edges define the flow of communication, as well as Dependency relation. Every Node has a Type and a Version. The version is currently implemented as a Timestamp as it's unique and increment only, naturally allowing for a conflict-free replicated data type (CRDT). The Graph Model is already used in abstract syntax trees to represent the structure of a program in compilers, except the added complexity of Directed Edges and Timestamps. 
* Directed Edges allow us to have better flow control, as well as define a one way type coercion between two Types. As a consequence, if there exists a path between type A and type B, type A can always be coerced to type B during the duration of a calculation (no more need of all that overloading, if we have all the information available at the call site, and there exists a path for all variables, coerce all of them to a combination that is defined and proceed with the calculation).
* Timestamps allows for automatic version propagation, as once a Node has changed its own state (after which its internal timestamp updates), dependen Nodes will be notified. One consequence of this is that Nodes are always updated to represent the current state.
* Black-box away programs into Nodes as an Abstraction mechanism
* A big consequence of Directed Edges and Timestamps is that it eliminates the need for build systems, version control, and programming becomes more about defining the proper relationship/dependency between programs
* Interactive programming 
* Nodes can live anywhere in the world, not necessarily on your own computer, allowing to share work with others, as well as interatively work on Nodes
* Nodes and Edges can be disconnected during runtime, allowing the programmer a lot of flexibility to experiment with combinations, or localize tests without using inflexible build systems
* Have a Node which gives us eyes to see the Graph and allow us to interface with the system with a frontend language. This language is a major part of this project. As a prototype this would be a simple graphical representation on the computer, probably opening up an editor to edit programs runtime. The only limit for the end goal is how well our brain is able to consume the information of what we are building and our imagination of what we are trying to build and experiment. So a big next step could be moving this language to Virtual Reality, as well as AI to build some connection with our brain as an interface to the system.

## What problem is the project trying to solve?
* Code reuse
* Dependency hell

## How does the project solve these problems?

## Current prerequisites of building
* C++ compiler

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
* Market research
* Build a simple but powerful enough prototype

