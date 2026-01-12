# Builder

Builder is a module‑oriented dependency resolver for workspaces composed of reusable modules. It is designed for fast iteration: compile quickly, validate what you learned, then combine with other modules. Builder is designed to be language‑agnostic; the only required files for a module are a JSON file that declares dependencies and a C++ plugin that implements the module’s build API.

## Contents

- [Quick start](#quick-start)
- [Module interface](#module-interface)
- [Artifacts, cycles and versioning](#artifacts-cycles-and-versioning)
- [Related repositories](#related-repositories)
- [Contributing](#contributing)
- [Requirements](#requirements)
- [License](#license)

## Quick start
For a concrete workspace, see [Builder‑Example](https://github.com/Gilqamesh/Builder-Example).

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

   Your plugin must define two C‑callable entry points:

   ```cpp
   extern "C" void builder__export_libraries(const builder_t* builder, bundle_type_t bundle_type);
   extern "C" void builder__import_libraries(const builder_t* builder);
   ```

   - `builder__export_libraries` builds and installs the modules libraries (static/shared)
   - `builder__import_libraries` links into final executables, at this stage all ordered libraries exists for the module to link against

   A typical plugin collects its source files, calls into the builder helpers to build and install into the target directory.

4. **Compile cli.cpp**

   ```bash
   make cli
   ```

5. **Run cli to build the target module**

   ```bash
   ./cli <modules_dir> <target_module> <artifacts_dir> [binary] [args...]
   ```

   | Argument        | Description                                                    |
   | --------------- | -------------------------------------------------------------- |
   | `modules_dir`   | Root directory containing all modules                          |
   | `target_module` | Name of the module to build                                    |
   | `artifacts_dir` | Output directory for versioned build artifacts                 |
   | `binary`        | *(Optional)* Path relative to the module’s `import/` directory |
   | `args...`       | *(Optional)* Arguments passed to the executed binary           |


   The CLI is self-hosting. If cli.cpp or the builder module changes, the CLI recompiles itself and re-execs automatically before continuing. This guarantees consistency between the CLI and the builder libraries.

## Artifacts and versioning

- Incremental build is on the module level, either a build is successful, or not. In case it's successful, cli deletes old versions to save space.
- Version of the module is determined by the latest modified timestamp of the module's source files recursively. The version will propagate to dependents, i.e., they will be rebuilt.

## Related repositories

- **Builder‑Example** – [example workspace](https://github.com/Gilqamesh/Builder-Example)
- **Builder‑Application‑1** – [long‑lived PoC workspace using Builder](https://github.com/Gilqamesh/Builder-Application-1)

## Contributing

- Open issues for bugs, questions and proposals.
- Open pull requests against the default branch with focused changes; describe expected vs actual behaviour and include how you tested the change.
- If you change user‑facing behaviour, update the README.

## Requirements

- CMake
- Clang C++ compiler with C++23 support
- Unix-like operating system

## License

MIT. See `LICENSE`.
