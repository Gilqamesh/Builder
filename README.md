# Builder

Builder is a tiny, modular, self-hosting C++ build system.

Every *module* lives in `modules/<name>/` and describes how it is built
in a single `builder.cpp` file. The top-level `builder` executable
(orchestrator) discovers modules, resolves dependencies by scanning
`#include <module/...>` in each `builder.cpp`, and runs the module
builders in topological order.

There are no global build files and no special DSL – just C++.

---

## Core Ideas

- **Modules, not projects**  
  Each directory under `modules/` is a module.  
  Example: `modules/builder/`, `modules/cpp_compiler/`, `modules/foo/`, …

- **Per-module builder**  
  Each module has a `builder.cpp` that is compiled into a small helper
  executable (the *builder artifact* for that module).  
  When the orchestrator runs that builder, it may produce any artifacts
  it wants: static libs, shared libs, executables, generated code, etc.

- **Dependencies via includes**  
  The orchestrator parses `builder.cpp` and looks for lines like:

  ```cpp
  #include <builder/api.h>      // depends on module "builder"
  #include <cpp_compiler/api.h> // depends on module "cpp_compiler"
  ```

  The token before the `/` inside `<…>` is treated as the module name
  and used to build the dependency graph.

- **Self-hosting**  
  The `builder` module itself is just another module under `modules/`.
  Its `builder.cpp` knows how to compile the orchestrator binary.  
  This means the system can rebuild itself using its own mechanisms.

---

## Layout

```text
<root>/
    builder                # orchestrator executable (built from modules/builder)
    modules/
        builder/
            builder.cpp    # builds the orchestrator itself
            main.cpp
            module.cpp
            api.h
            ...
        cpp_compiler/
            builder.cpp    # e.g. wraps g++/clang, manages .o/.a/.so
            api.h
            ...
        <other_module>/
            builder.cpp
            ...
    artifacts/             # all module artifacts live here
```

- `modules/<name>/builder.cpp` — how this module is built
- `artifacts/` — outputs; each module is free to define its own naming
  (often via a small helper API)

---

## Orchestrator CLI

The root `builder` executable is the entrypoint.

Basic usage:

```bash
./builder <module_name>
```

It assumes:

- modules live under `./modules`
- artifacts go under `./artifacts`

You can override these with flags:

```bash
./builder [-B <artifacts_root>] [-S <modules_root>] <module_name>
```

- `-S` — path to the modules root (default: `modules`)
- `-B` — path to the artifacts root (default: `artifacts`)
- `<module_name>` — the module you want to build

The orchestrator will:

1. Discover all modules in `<modules_root>`.
2. Find their `builder.cpp` files.
3. Build and run `builder` artifacts in dependency order.
4. Stop on the first error.

---

## Builder API (per-module)

Modules that want a convenient view of the environment can use a minimal
header-only API, typically included as:

```cpp
#include <builder/api.h>
```

One example shape:

```cpp
builder_api_t api = builder_api_t::from_argv(argc, argv);

auto modules_root   = api.modules_root();
auto artifacts_root = api.artifacts_root();
auto module_name    = api.module_name();
auto module_dir     = api.module_dir();
```

The API’s job is deliberately small:

- expose paths (modules root, artifacts root, current module name)
- provide a conventional way to construct artifact paths *if desired*

Each module is still free to invent its own naming scheme (e.g.
`<name>.static_library`, `<name>.shared_library`, `<name>.executable`,
or something custom).

---

## Example: a Simple Module

`modules/foo/builder.cpp`:

```cpp
#include <builder/api.h>

#include <format>
#include <iostream>

int main(int argc, char** argv) {
    builder_api_t api = builder_api_t::from_argv(argc, argv);

    auto exe_path = api.artifacts_root() / (api.module_name() + ".executable");
    auto src      = api.module_dir() / "foo.cpp";

    std::string cmd = std::format(
        "g++ -g -O2 -std=c++23 -Wall -Wextra -I{} -o {} {}",
        api.modules_root().string(),
        exe_path.string(),
        src.string()
    );

    std::cout << cmd << std::endl;
    int rc = std::system(cmd.c_str());
    if (rc != 0) {
        throw std::runtime_error(std::format(
            "module {} failed with exit code {}",
            api.module_name(), rc
        ));
    }

    return 0;
}
```

This builder:

1. Is compiled by the orchestrator into an internal “builder artifact”.
2. When run, it compiles `foo.cpp` into `artifacts/foo.executable`.

Other modules can then treat `foo` however they like (link it, run it,
etc.).

---

## Philosophy

- **Minimal API** – as little shared machinery as possible
- **Local reasoning** – each module is self-contained
- **Topological builds** – dependencies first, then dependents
- **Self-hosting** – the build system can build itself
- **Hackable** – everything is just C++; no extra DSL to learn

---

## License

Builder is released under the MIT License. See `LICENSE` for details.
