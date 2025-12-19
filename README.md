# Builder

Builder is a module-oriented build orchestrator for workspaces composed of reusable **modules**.
It is designed to reduce coordination overhead, keep build logic colocated with the module it builds,
and enable fast iteration (including “build it, learn it, shelve it”).

A module can be language-agnostic. The only Builder-facing surface is:
- `deps.json` (declares dependencies)
- `builder_plugin.cpp` (implements the build protocol in C++)

Builder is invoked as a standalone executable. It does not impose fixed directory names
or a fixed workspace layout.


---

## Quick start

### 1) Create a module

A module is simply a directory inside your chosen `<modules>` directory.

`<modules>/<module>/deps.json`

```json
{
  "builder_deps": ["builder"],
  "module_deps": []
}
```

`<modules>/<module>/builder_plugin.cpp`

```cpp
#include "builder_plugin.h"

extern "C" void builder__build_self(builder_ctx_t*, const builder_api_t*) {
    // Compile sources and emit static library artifacts for this module.
}

extern "C" void builder__build_module(builder_ctx_t*, const builder_api_t*, const char* static_libs) {
    // Build final module outputs, linking against `static_libs` in the given order.
}
```

### 2) Compile Builder Driver

Builder Driver is compiled like a normal C++ program.

The parent directory of `<modules>` is used as the include namespace so that modules
can include each other consistently, e.g.:

```cpp
#include <modules/moduleA/moduleA.h>
```

Example compilation:

```bash
clang++ -std=c++23 <modules_dir>/*.cpp -I$(dirname <modules_dir>) -o builder_driver
```

### 3) Run Builder Driver

```
./builder_driver <modules_dir> <target_module> <artifacts_dir>
```

Builder discovers modules under `<modules_dir>`, plans the build, and installs
artifacts under `<artifacts_dir>`.


---

## Module interface

### `deps.json`

Each module declares two dependency sets:

- `builder_deps`  
  Modules required to compile and link the module’s **builder plugin**.

- `module_deps`  
  Modules whose static libraries or produced artifacts are inputs to this module’s build.

Dependency information is explicit; Builder does not infer dependencies from source code.

### `builder_plugin.cpp`

Each module must export exactly two C-callable entry points:

```cpp
extern "C" void builder__build_self(builder_ctx_t*, const builder_api_t*);
extern "C" void builder__build_module(builder_ctx_t*, const builder_api_t*, const char* static_libs);
```

Contract:

- `builder__build_self`
  - compiles the module’s sources
  - emits one or more **static libraries**
  - static libraries compiled as **PIC** may be linked into other builder plugins
    (because builder plugins are shared objects)
  - non-PIC static libraries may still be reused at the final link stage, but
    cannot be embedded into builder plugins

- `builder__build_module`
  - produces the module’s final artifacts (executables, shared libraries, etc.)
  - installs those artifacts into the module’s artifact directory
  - receives all required static libraries as a **single ordered string**
  - performs no dependency discovery or ordering logic

`builder.h` provides helper utilities for common tasks such as compiling and
archiving static libraries, but plugins remain free to invoke the toolchain directly.


---

## Artifacts, cycles, and versioning

### Transactional artifacts

Builds are executed transactionally:

- artifacts are either fully built and installed, or not installed at all
- partial outputs are discarded
- failed builds never pollute the artifact store

By default, older artifact versions are deleted to save space.

### Circular dependencies (Strongly Connected Components)

Circular dependencies between modules are allowed.

When modules form a **strongly connected component (SCC)**:
- they are treated as a single unit for linking
- their static libraries are bundled into one combined static library
- the bundle is installed under:

```
<artifacts>/scc/<hash>/<version>/
```

This is transparent to module authors; no special handling is required.

### Source-based versioning (current)

- all file modification times under a module are scanned
- the maximum timestamp determines the module version
- if the corresponding artifact directory does not exist, the module is rebuilt

Versioning propagates through dependencies to ensure a single consistent view of
the workspace.

Authored versioning (e.g. semantic versions) may be added later.


---

## Related repositories


- **Builder-Example**
  A small collection of example modules demonstrating how to use Builder.

- **Builder-Application-1**
  A long-lived repository of modules and applications developed using Builder.

---

## Requirements

- POSIX-like operating system
- C++23-capable compiler
- standard Unix toolchain

---

## License

MIT. See `LICENSE`.
