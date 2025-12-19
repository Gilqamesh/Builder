# Builder

Builder is a **module-oriented build orchestrator** for workspaces composed of reusable modules. It optimizes for fast iteration: build quickly, validate what you learned, and discard experiments without overhead.

A module is language-agnostic. The Builder-facing surface is:
- `deps.json` — declares dependencies
- `builder_plugin.cpp` — implements the build protocol in C++

Builder runs as a standalone executable. It does **not** impose fixed directory names or a fixed workspace layout.

---

## Contents
- [Quick start](#quick-start)
- [Module interface](#module-interface)
- [Artifacts, cycles, and versioning](#artifacts-cycles-and-versioning)
- [Related repositories](#related-repositories)
- [Requirements](#requirements)
- [License](#license)

---

## Quick start

### 1) Create a module
A module is a directory under your chosen `<modules>` directory.

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
Builder Driver is compiled like a normal C++ program. The parent directory of `<modules>` is used as the include namespace so modules can include each other consistently, e.g.:
```cpp
#include <modules/moduleA/moduleA.h>
```

Example compilation:
```bash
clang++ -std=c++23 <modules_dir>/*.cpp -I$(dirname <modules_dir>) -o builder_driver
```

### 3) Run Builder Driver
```bash
./builder_driver <modules_dir> <target_module> <artifacts_dir>
```
Builder discovers modules under `<modules_dir>`, plans the build, and installs artifacts under `<artifacts_dir>`.

---

## Module interface

### `deps.json`
Two dependency sets per module:
- `builder_deps`: modules required to compile/link the module’s **builder plugin**.
- `module_deps`: modules whose static libraries or produced artifacts are inputs to this module’s build.

### `builder_plugin.cpp`
Each module exports exactly two C-callable entry points:
```cpp
extern "C" void builder__build_self(builder_ctx_t*, const builder_api_t*);
extern "C" void builder__build_module(builder_ctx_t*, const builder_api_t*, const char* static_libs);
```

Contract:
- `builder__build_self`
  - compiles the module’s sources
  - emits one or more **static libraries**
  - static libraries compiled as **PIC** may be linked into builder plugins (shared objects)
  - non-PIC static libraries may still be reused at the final link stage but cannot be embedded into builder plugins
- `builder__build_module`
  - produces final artifacts (executables, shared libraries, etc.)
  - installs artifacts into the module’s artifact directory
  - receives all required static libraries as a **single ordered string**
  - performs no dependency discovery or ordering logic

`builder.h` provides helpers for common tasks (compiling, archiving), but plugins may invoke the toolchain directly.

---

## Artifacts, cycles, and versioning

### Transactional artifacts
- Artifacts are either fully built and installed, or not installed at all.
- Partial outputs are discarded; failed builds never pollute the artifact store.
- Older artifact versions are removed by default to save space.

### Circular dependencies (SCCs)
Circular dependencies are allowed. A **strongly connected component (SCC)** is treated as a single unit for linking:
- static libraries are bundled into one combined static library
- the bundle is installed under:
  ```
  <artifacts>/scc/<hash>/<version>/
  ```
This is transparent to module authors.

### Source-based versioning (current)
- All file modification times under a module are scanned; the maximum timestamp determines the module version.
- If the corresponding artifact directory does not exist, the module is rebuilt.
- Versioning propagates through dependencies to ensure a single consistent workspace view.
- Authored versioning (e.g., semantic versions) may be added later.

---

## Related repositories
- **Builder-Example** — A small collection of example modules demonstrating how to use Builder.
- **Builder-Application-1** — A long-lived repository of modules and applications developed using Builder.

---

## Requirements
- POSIX-like operating system
- C++23-capable compiler
- Standard Unix toolchain

---

## License
MIT. See `LICENSE`.