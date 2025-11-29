# Builder

**Builder** is a minimal, fast, self-contained C++ build system for
experimenting with ideas through small, composable modules.

Each module lives in `src/<module>/` and defines its behavior in a
simple `build.module` file.\
The `builder` executable reads these descriptions, resolves
dependencies, performs automatic header scanning, globs sources, and
executes incremental builds.

------------------------------------------------------------------------

## Why Builder?

-   **No CMake, Meson, or Bazel**
-   **No workspaces or top-level build files**
-   **Every module self-describes**
-   **Extremely fast incremental rebuilds**
-   **Automatic header dependency scanning**
-   **Recursive source globbing**
-   **Seamless consumption of external CMake projects (`remote_cmake`)**
-   **Simple & hackable --- a single \~1000-line C++ file**

------------------------------------------------------------------------

## Layout

    <root>/builder.cpp        # The build tool
    <root>/src/<module>/      # Each module has:
        build.module          #   - its own build descriptor
        *.cpp / *.h / ...     #   - sources
    <root>/build/<module>/    # Build artifacts
    <root>/.cache/remote/     # Cached remote CMake projects

------------------------------------------------------------------------

## Using Builder

Compile the tool:

``` bash
g++ -std=c++17 -O2 builder.cpp -o builder -Wall -Wextra -Werror
```

Build a module:

``` bash
./builder <module_name>
```

For example:

``` bash
./builder function_visualizer
```

------------------------------------------------------------------------

## Example `build.module`

    MODULE_NAME=function_visualizer
    MODULE_TYPE=executable
    MODULE_SOURCES="function_visualizer.cpp "
    MODULE_DEPS=" function_visualizer_editor function_repository function_alu function_compound function_ir_file_repository rlImGui"
    MODULE_SYS_LIBS=""
    MODULE_CXX_FLAGS="-std=c++23 -Wall -Wextra -O2"
    MODULE_LD_FLAGS=""

### Remote CMake modules

    MODULE_NAME=raylib
    MODULE_TYPE=remote_cmake
    MODULE_FETCH_URL="https://github.com/raysan5/raylib/archive/refs/tags/5.5.tar.gz"
    MODULE_DEPS=""

------------------------------------------------------------------------

## Adding a New Module

1.  Create a directory: `src/<module>/`
2.  Add a `build.module` file describing:
    -   name, type, source patterns, deps, flags, etc.
3.  Add your `.cpp`/`.h` files
4.  Build it:

``` bash
./builder <module>
```

------------------------------------------------------------------------

## Philosophy

Builder aims to be:

-   **Tiny** --- one C++ file you can fully understand\
-   **Deterministic** --- predictable builds and dependency resolution\
-   **Hackable** --- simple enough to extend or modify freely\
-   **Exploratory** --- ideal for rapid prototyping, language
    experiments, and modular architectures

------------------------------------------------------------------------

## License

Builder is released under the MIT License; see LICENSE for details.
