# Builder

Builder is a C++ build tool for workspaces composed of reusable modules. Module dependencies are declared explicitly, while build behavior is implemented directly in C++.

## Contents

- [Problem statement](#problem-statement)
- [Design](#design)
- [Workspace layout](#workspace-layout)
- [Quick start](#quick-start)
- [Related repositories](#related-repositories)
- [Contributing](#contributing)
- [Requirements](#requirements)
- [License](#license)

## Problem statement

Most existing C++ build systems describe build behavior using a dedicated build language or configuration format, rather than C++ itself.

Builder aims to provide a build system that can:
- Express build logic entirely in C++
- Avoid a separate build DSL
- Still provide deterministic dependency resolution, ordering, and incremental rebuilds

## Design

Builder separates structural dependency information from build behavior.

- Module dependencies are declared declaratively in `deps.json`
- Build behavior is implemented in C++ via a fixed set of build entry points (`builder.cpp`) that the build tool invokes in a well-defined order
- The CLI is self-hosting: if the builder module changes, the CLI rebuilds and re-execs itself to ensure it always matches the current builder version
- Dependency cycles are handled by building strongly connected components as a unit
- Incremental rebuilds operate at module granularity; module versions are derived from source timestamps and propagate transitively
- Obsolete artifact versions are removed after successful builds
- For each module, the build tool maintains an `alias` directory that points to the latest successfully built artifact version

## Workspace layout

Builder operates on a workspace defined by two directories:

- A modules directory, containing all modules in the workspace
- An artifacts directory, where build outputs are written

The Builder repository itself is expected to be present as a module inside the modules directory. After an initial bootstrap step, it is built and versioned like any other module.

## Quick start
For an example workspace using Builder, see [Builder-Example].

1. **Create a module**

   A module lives under a directory in your `<modules>` root. Each module needs two files:

   * `deps.json` – lists the module dependencies.
   * `builder.cpp` – implements the build protocol described below.

2. **Write `deps.json`**

   The `deps` array lists modules your module depends on.

   ```json
   {
     "deps": ["math", "physics", "visualizer", "engine", "core"]
   }
   ```

3. **Implement `builder.cpp`**

   Your plugin must define three C‑callable entry points:

   ```cpp
   extern "C" void builder__export_interface(const builder_t* builder, library_type_t library_type);
   extern "C" void builder__export_libraries(const builder_t* builder, bundle_type_t bundle_type);
   extern "C" void builder__import_libraries(const builder_t* builder);
   ```

   - `builder__export_interface` installs the module interfaces (i.e., headers)
   - `builder__export_libraries` builds and installs the module libraries (static/shared)
   - `builder__import_libraries` links into final executables, at this stage all ordered libraries exist for the module to link against

   A typical plugin collects its source files, calls into the builder helpers to build and install into the target directory.

4. **Bootstrap the builder module**

   ```bash
      make -C <modules_dir>/builder -f bootstrap.mk bootstrap MODULES_DIR=<modules_dir> ARTIFACTS_DIR=<artifacts_dir>
   ```

5. **Run the cli to build the target module**

   ```bash
   <artifacts_dir>/builder/alias/import/install/cli <modules_dir> <target_module> <artifacts_dir> [binary] [args...]
   ```

   | Argument        | Description                                                    |
   | --------------- | -------------------------------------------------------------- |
   | `modules_dir`   | Root directory containing all modules                          |
   | `target_module` | Name of the module to build                                    |
   | `artifacts_dir` | Output directory for versioned build artifacts                 |
   | `binary`        | *(Optional)* Path relative to the module’s `import/` directory |
   | `args...`       | *(Optional)* Arguments passed to the executed binary           |

## Related repositories

- **Builder‑Example** – [example workspace](https://github.com/Gilqamesh/Builder-Example)
- **Builder‑Application‑1** – [long‑lived PoC workspace using Builder](https://github.com/Gilqamesh/Builder-Application-1)

## Contributing

- Open issues for bugs, questions and proposals
- Open pull requests against the default branch with focused changes; describe expected vs actual behaviour and include how you tested the change
- If you change user‑facing behaviour, update the README

## Requirements

- Unix-like operating system
- Binaries:
   - `/usr/bin/cmake`
   - `/usr/bin/clang++` (with `-std=c++23` support)
   - `/usr/bin/clang`
   - `/usr/bin/ar`

- The following libraries:
   - `/usr/lib64/libcurl.so`

## License

See [LICENSE](LICENSE).
