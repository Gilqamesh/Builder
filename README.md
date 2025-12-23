# Builder

Builder is a **module‑oriented build orchestrator** for workspaces composed of reusable modules.  It is designed for fast iteration: compile quickly, validate what you learned, then discard experiments without the overhead typical of monolithic build systems.  Builder is **language‑agnostic**; the only required files for a module are a JSON file that declares dependencies and a C++ plugin that implements the module’s build API.

## Contents

- [Quick start](#quick-start)
- [Module interface](#module-interface)
- [Artifacts, cycles and versioning](#artifacts-cycles-and-versioning)
- [Related repositories](#related-repositories)
- [Contributing](#contributing)
- [Requirements](#requirements)
- [License](#license)

## Quick start

1. **Create a module**

   A module lives under a directory in your `<modules>` root.  Each module needs two files:

   * `deps.json` – describes **builder dependencies** (modules required to build the plugin) and **module dependencies** (modules whose exported bundles are inputs to your link stage).
   * `builder_plugin.cpp` – implements the build protocol described below.

2. **Write `deps.json`**

   The `builder_deps` array lists modules your plugin depends on, and `module_deps` lists modules whose static/shared bundles should be passed to your link phase:

   ```json
   {
     "builder_deps": ["builder"],
     "module_deps": []
   }
   ```

3. **Implement `builder_plugin.cpp`**

   Your plugin must define three C‑callable entry points:

   ```cpp
   extern "C" void builder__export_bundle_static(builder_ctx_t*, const builder_api_t*);
   extern "C" void builder__export_bundle_shared(builder_ctx_t*, const builder_api_t*);
   extern "C" void builder__link_module(builder_ctx_t*, const builder_api_t*);
   ```

   - `builder__export_bundle_static` packages your module’s static libraries (PIC or non‑PIC) into a bundle that other modules can link against.
   - `builder__export_bundle_shared` packages a set of shared libraries for linking by dependent modules.
   - `builder__link_module` links your module’s final artifacts (executables, other file formats, etc.) with access to the ordered link command line for its dependencies.

   The [`builder_api_t`] interface passed into these functions exposes helpers for locating directories and generating linker command lines:

   - `modules_dir(ctx)` – root directory containing all modules.
   - `source_dir(ctx)` – the module’s source directory.
   - `static_bundle_dir(ctx)` – directory in the artifact store where your static bundle should be installed.
   - `shared_bundle_dir(ctx)` – directory in the artifact store where your shared bundle should be installed.
   - `linked_artifacts_dir(ctx)` – directory where your final linked artifacts should be installed.
   - `get_static_link_command_line(ctx)`/`get_shared_link_command_line(ctx)` – return command‑line strings giving the ordered list of static/shared bundles for dependencies.

   A typical plugin collects its source `.cpp` files, calls `builder_t::materialize_static_library` or `builder_t::materialize_shared_library` to produce bundles, then calls `builder_t::materialize_binary` to link the final binary using the command line provided by `builder_api_t`.

4. **Compile the Builder driver**

   Compile `builder_driver.cpp` along with your module implementations using a C++23 compiler.  The parent of `<modules>` must be on the include path so modules can include one another, e.g.:

   ```bash
   clang++ -std=c++23 <modules_dir>/builder/*.cpp -I$(dirname <modules_dir>) -o builder_driver
   ```

5. **Run the build**

   Run the driver with the modules directory, target module and output artifacts directory:

   ```bash
   ./builder_driver <modules_dir> <target_module> <artifacts_dir>
   ```

   Builder discovers modules, computes a dependency graph, plans the build and installs bundles and final artifacts into `<artifacts_dir>`.

For a concrete workspace and helper scripts see [Builder‑Example](https://github.com/Gilqamesh/Builder-Example).

## Module interface

### `deps.json`

Each module declares two dependency sets:

- **builder_deps** – modules required to compile and link the module’s *builder plugin*.
- **module_deps** – modules whose exported bundles are inputs to this module’s link phase.

### `builder_plugin.cpp`

Each module must export three functions as shown above.  A well‑behaved plugin must obey the following contract:

- **`builder__export_bundle_static`**  
  Package all static libraries produced by your module (including PIC and non‑PIC variants) into a bundle so that dependent modules can link against them.  Throw an exception if your module does not produce static libraries.

- **`builder__export_bundle_shared`**  
  Package the complete set of shared libraries produced by your module.  Throw an exception if your module does not produce shared libraries.

- **`builder__link_module`**  
  Produce final artifacts (executables, shared objects, etc.) and install them under `linked_artifacts_dir(ctx)`.  Use the ordered static or shared link command lines returned by the API to link the correct dependencies.

The `builder_api_t` class provides helper methods for common tasks and asserts that prerequisite bundles are available (via `assert_has_static_libraries`, `assert_has_shared_libraries` and `assert_has_linked_artifacts`).  Static helpers in `builder_t` simplify compiling source files, producing archives and linking executables.

## Artifacts, cycles and versioning

- **Transactional artifacts** – builds are transactional: artifacts are installed only on success.  Partial outputs are discarded; failed builds never pollute the artifact store.
- **Circular dependencies** – Builder supports circular dependencies.  Strongly connected components (SCCs) of the dependency graph are treated as single units.  The static libraries from all modules in an SCC are bundled together and stored under `scc/<hash>/<version>/`.  This bundling is transparent to module authors.
- **Source‑based versioning** – the version of a module is determined by the latest modification timestamp of any file under the module.  A module is rebuilt when its timestamp increases.  Versioning propagates through dependencies to ensure a consistent view of the workspace.  Authored versioning (e.g., semantic versions) may be added in the future.

## Related repositories

- **Builder‑Example** – [example workspace and helper scripts](https://github.com/Gilqamesh/Builder-Example)
- **Builder‑Application‑1** – [long‑lived application workspace built on Builder](https://github.com/Gilqamesh/Builder-Application-1)

## Contributing

- Open issues for bugs, questions and proposals.
- Open pull requests against the default branch with focused changes; describe expected vs actual behaviour and include how you tested the change.
- If you change user‑facing behaviour (flags, outputs, plugin hooks), update the README or relevant documentation.

## Requirements

- POSIX‑like operating system
- C++23‑capable compiler
- Standard Unix toolchain (e.g. `clang++`, `ar`, `ld`)

## License

MIT. See `LICENSE`.
