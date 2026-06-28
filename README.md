# Builder

Builder is a C++ build framework organized around modules.

The usual workflow is:

1. Write a C++ module.
2. Write the module's `builder.cpp`.
3. Declare dependencies in `deps.json`.
4. Run the module with `./cli <module> [args...]`.

## Prerequisites

Builder currently targets a Linux or Linux-compatible POSIX environment. The
default bootstrap makefile expects `clang++`, `clang`, `ar`, `ln`, `mkdir`, `mv`,
and `rm` under `/usr/bin`, a C++23-capable compiler, and `libdl`.

## Quick Start

Bootstrap the local Builder CLI:

```sh
make bootstrap
```

This creates the initial Builder CLI and builder plugin used to rebuild the
modules that implement Builder itself.

Run existing modules:

```sh
./cli m03gagbhtft23yhjwpp881tfmc_uuidv7 --uuid
./cli m03gagbht2l61mj6qitacwbmea_base36 0100
```

Create a new application module:

```sh
./cli m03gagbht5685jfnokvj7crv2c_create_module <workspace> hello_module
```

## Module Command

The normal command is:

```sh
./cli <module> [args...]
```

`<module>` is a module name, not a path. Builder discovers that module, builds
its default CLI if needed, then execs that CLI with `[args...]`.

## Environment Variables

Builder uses two environment variables to find source and output locations:

- `BUILDER_WORKSPACE_ROOT`: directory containing `workspaces.json`; defaults to
  the current directory.
- `BUILDER_ARTIFACT_ROOT`: directory where Builder writes generated files;
  defaults to
  `<BUILDER_WORKSPACE_ROOT>/artifacts`.

Builder sets these variables before running a module CLI, so the CLI and its
child processes see the same workspace and artifact locations.

The initial Builder plugin created by `make bootstrap` is recorded in the
compiled Builder. `BUILDER_ARTIFACT_ROOT` moves outputs for the current run; it
does not relocate that bootstrap plugin.

## What is a module?

A module is the unit Builder builds and runs. It owns:

- source files;
- `deps.json`;
- `builder.cpp`;
- build output;
- public headers;
- linkable library capability;
- default CLI capability.

A module directory is:

```text
<workspace>/<module_name>/
```

Module names must be globally unique because `deps.json` and `./cli <module>`
refer to modules by name.

### Module Naming

This repository uses generated module names with a durable identity prefix and a
human-readable semantic slug:

```text
m<zero-padded-base36-uuidv7>_<semantic_slug>
```

For example:

```text
m03gagbht2l61mj6qitacwbmea_base36
```

The whole string is the module identity. The generated prefix keeps module names
globally unique across workspaces and repositories. The semantic slug makes the
module easier to recognize in includes, dependency lists, commands, and
diagnostics, but it is still part of the identity.

## What are workspaces?

Workspaces define which modules may depend on which other modules.

`workspaces.json` lists workspaces from lower-level to higher-level.

The rules are:

- module source may depend on modules in the same workspace or a lower
  workspace;
- `builder.cpp` may depend only on modules in lower workspaces.

The active Builder bootstrap group is the narrow exception. Modules that
implement the running Builder can use same-workspace builder dependencies so the
local bootstrap seed can build Builder itself without a lower bootstrap
workspace. That exception is not general module authoring semantics, and it does
not make builder dependency cycles legal outside the active Builder bootstrap
group.

## What is `deps.json`?

Each module declares two dependency lists in `deps.json`:

```json
{
    "module_dependencies": [],
    "builder_dependencies": []
}
```

`module_dependencies` are used by the module's C++ source.

`builder_dependencies` are used by `builder.cpp`. Builder exposes those
dependencies to `builder.cpp` as normal C++ include and link inputs.

Declare and include the modules you reference directly. If source code, a public
header, or `builder.cpp` names a module API, that file includes the module's
public header and the module appears in the relevant dependency list. Do not
rely on transitive includes, transitive dependency declarations, or forward
declarations of another module's API to make your code compile.

Dependencies outside these rules fail before build output is produced.

Same-workspace module dependency cycles are handled as strongly connected
component groups. The group graph is ordered topologically from dependency to
dependent, and every module in a group can publish interfaces before library
compilation. Builder dependency cycles are not modeled as module groups.

For example, a tool module that uses the build phase API and filesystem path API
in `builder.cpp`, and uses `base36` in its own source, would declare:

```json
{
    "module_dependencies": [
        "m03gagbht2l61mj6qitacwbmea_base36"
    ],
    "builder_dependencies": [
        "m03gagbhsujjf63n0w3r2w4q6h_build_phases",
        "m03gagbhsnusi43zogoacgj2ez_filesystem"
    ]
}
```

## What are build phases?

A build configuration selects library type and phase order. The default phase
order is:

```text
source -> interface -> library -> binary
```

The phases are:

- `source`: make source-side inputs available;
- `interface`: publish public headers;
- `library`: publish linkable library output;
- `binary`: publish executable output, including the default CLI.

`builder.cpp` includes the phase API and defines one C-callable function per
phase.

```cpp
#include <m03gagbhsujjf63n0w3r2w4q6h_build_phases/build_phases.h>

extern "C" void phase__source(const m03gagbhsujjf63n0w3r2w4q6h_build_phases::source_phase_t* phase) {
    phase->install_source_tree();
}

extern "C" void phase__interface(const m03gagbhsujjf63n0w3r2w4q6h_build_phases::interface_phase_t* phase) {
    phase->install_headers_from_source();
}

extern "C" void phase__library(const m03gagbhsujjf63n0w3r2w4q6h_build_phases::library_phase_t* phase) {
}

extern "C" void phase__binary(const m03gagbhsujjf63n0w3r2w4q6h_build_phases::binary_phase_t* phase) {
}
```

Builder owns graph order, group order, and phase order. A builder only describes
local work for the phase it is given.

Each phase has two roots:

- `build_dir()`: private scratch for the current phase;
- `install_dir()`: public completed output for later phases.

Generated files and compiler outputs belong under `build_dir()`. Published
paths are copied into `install_dir()`.

Important phase APIs:

- `build(path)`: selects a path from an earlier phase `install_dir()` for
  compile/link helpers;
- `install(path)`: publishes a path into the current phase `install_dir()` under
  the same relative path. The path must come from the current `build_dir()` or
  an earlier phase `install_dir()`. Installing two paths to the same relative
  path is an error.
- `install<T>()`: ensures phase `T` is installed and returns its result.

`build_library(...)` and `build_cli(...)` accept compile defines for the sources
being compiled. Compile define keys are validated by
`m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::define_t` and should use a
macro-safe module identity prefix when they are module-owned. Compile defines
are private compile inputs; public module headers should not expose private
macros unless the macro is deliberately part of the public API. External host
tools should be validated when the code that uses them runs, not while
installing unrelated phases.

### Public Includes

Public includes are module-qualified:

```cpp
#include <module_name/header.h>
```

For a module named `m03gagbht2l61mj6qitacwbmea_base36` with `base36.h`,
`install_interface(...)` publishes this include:

```cpp
#include <m03gagbht2l61mj6qitacwbmea_base36/base36.h>
```

Imported upstream packages sometimes include their own headers without Builder's
module prefix. `install_interface_compatibility(...)` publishes the extra include
root needed for those upstream self-includes. Module users should still include
through the module name.

Public headers that use include guards should derive the guard from the module
name and the module-relative installed header path.

## What is `builder.cpp`?

`builder.cpp` is the module's C++ build file. Its job is to build that module's
own outputs: published headers, libraries, generated files, imported files, and
binaries.

It is regular C++ code. If `builder.cpp` needs helper code, tools, or libraries,
the module lists them in `builder_dependencies`. Builder then makes those
dependencies available to `builder.cpp` through normal C++ includes and linking.

Each phase function in `builder.cpp` describes only the local work for that
phase. For example, it can install headers, compile source files, generate a
file under `build_dir()`, run an imported tool, or publish the module's default
CLI.

To build a library, replace the empty `phase__library` function above with one
that compiles a source file from the source phase `install_dir()` and publishes
the library:

```cpp
#include <m03gagbhsujjf63n0w3r2w4q6h_build_phases/build_phases.h>
#include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>

extern "C" void phase__library(const m03gagbhsujjf63n0w3r2w4q6h_build_phases::library_phase_t* phase) {
    const auto sources = phase->install<m03gagbhsujjf63n0w3r2w4q6h_build_phases::source_phase_t>();
    const auto library = phase->build_library({
        phase->build(sources.root() / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("base36.cpp") )
    }, {});
    phase->install_library(library);
}
```

## Artifacts

Builder writes generated files under `BUILDER_ARTIFACT_ROOT`.

Artifacts are grouped by module name, but the exact directory layout is an
implementation detail:

```text
<BUILDER_ARTIFACT_ROOT>/<module_name>/
```

Builder exposes a stable `latest` view for tooling and debugging:

```text
<BUILDER_ARTIFACT_ROOT>/<module_name>/latest/
```

`latest` is a stable path for external tooling, but it is not dependency
versioning or a module authoring API. Module builders should use phase APIs such
as `build_dir()`, `build(path)`, `install(path)`, and `install<T>()` instead of
hard-coding artifact paths or inspecting phase install roots directly.

## Main Builder Modules

- `m03gagbhsp2drqq3gkop8pzfrm_workspace_graph`: discovers modules, loads
  `deps.json`, validates workspace order, forms module dependency strongly
  connected component groups, and propagates versions.
- `m03gagbhsujjf63n0w3r2w4q6h_build_phases`: defines phase APIs, lazy
  installation, phase roots, interface publication, library builds, binary
  builds, and default CLI fallback.
- `m03gagbhst621faiop1rztfkqp_builder_cli`: implements
  `./cli <module> [args...]`, CLI self-update, target binary installation, and
  process execution.
- `m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain`: compiles objects, archives static
  libraries, links shared libraries, links binaries, and injects include/link
  inputs.

## Long-term goals

- Support versioned module dependencies.
- Support composing modules by name through a small LISP-like expression language.
