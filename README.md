# Builder

Builder is a module‑oriented dependency resolver for workspaces composed of reusable modules. It is designed for fast iteration: compile quickly, validate what you learned, then discard experiments without the overhead typical of monolithic build systems. Builder is designed to be **language‑agnostic**; the only required files for a module are a JSON file that declares dependencies and a C++ plugin that implements the module’s build API.

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

   Your plugin must define two C‑callable entry points:

   ```cpp
   extern "C" void builder__export_libraries(builder_ctx_t* ctx, const builder_api_t* api, bundle_type_t bundle_type);
   extern "C" void builder__build_module(builder_ctx_t* ctx, const builder_api_t* api);
   ```

   - `builder__export_libraries` builds and installs the modules libraries (static/shared)
   - `builder__build_module` links into final executables, at this staged all ordered libraries exists for the module to link against

   A typical plugin collects its source files, calls into the builder helpers to build and install into the target directory.

4. **Compile cli.cpp**

   ```bash
   clang++ -std=c++23 *.cpp -o cli
   ```

5. **Run cli to build the target module**

   ```bash
   ./cli <builder_dir> <modules_dir> <target_module> <artifacts_dir>
   ```

## Artifacts, cycles and versioning

- **Transactional artifacts** – builds are transactional: artifacts are installed only on success.  Partial outputs are discarded; failed builds never pollute the artifact store.
- **Circular dependencies** – Builder supports circular dependencies.  Strongly connected components (SCCs) of the dependency graph are treated as single units.  The static libraries from all modules in an SCC are bundled together and stored under `scc/<hash>/<version>/`.  This bundling is transparent to module authors.
- **Source‑based versioning** – the version of a module is determined by the latest modification timestamp of any file under the module.  A module is rebuilt when its timestamp increases.  Versioning propagates through dependencies to ensure a consistent view of the workspace.  Authored versioning (e.g., semantic versions) may be added in the future.

## Related repositories

- **Builder‑Example** – [example workspace](https://github.com/Gilqamesh/Builder-Example)
- **Builder‑Application‑1** – [long‑lived PoC workspace using Builder](https://github.com/Gilqamesh/Builder-Application-1)

## Contributing

- Open issues for bugs, questions and proposals.
- Open pull requests against the default branch with focused changes; describe expected vs actual behaviour and include how you tested the change.
- If you change user‑facing behaviour (flags, outputs, plugin hooks), update the README or relevant documentation.

## Requirements

- C++23 compiler
- Unix environment

## License

MIT. See `LICENSE`.
