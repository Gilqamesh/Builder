# Builder project layout

This repository is organized around standalone programs that live under `programs/`. Each program has its own CMake file placed directly in that directory and pulls the source files it needs from `builder/`, so programs can be added, removed, or modified independently. Helper build scripts in `scripts/` call the program-specific files from any working directory.

## Key directories
- `builder/`: Core modules that programs combine to form different binaries.
- `programs/`: Program entrypoints defined by individual CMake files, each with its own `external/` directory for third-party sources.
- `scripts/`: Helper scripts for configuring and building specific programs.

## Building
Use the script that corresponds to the program you want to build. For example:
- `./scripts/build_tests.sh`
- `./scripts/build_visualizer.sh`

These scripts create their own build directories under `build/` and do not rely on a top-level CMake configuration. Programs stay modular: each target selects the builder components it needs, can swap in new modules, and keeps its external dependencies alongside its own sources.
