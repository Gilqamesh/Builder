# Builder project layout

This repository is organized around standalone programs that live under `programs/`. Each program has its own CMake file placed directly in that directory and pulls the source files it needs from `builder/`, so programs can be added, removed, or modified independently. Helper build scripts in `scripts/` call the program-specific files from any working directory.

## Key directories
- `builder/`: All core source files shared by programs.
- `programs/`: Program entrypoints defined by individual CMake files (for example, `library.cmake`).
- `scripts/`: Helper scripts for configuring and building specific programs.

## Building
Use the script that corresponds to the program you want to build. For example:
- `./scripts/build_library.sh`
- `./scripts/build_tests.sh`
- `./scripts/build_visualizer.sh`

These scripts create their own build directories under `build/` and do not rely on a top-level CMake configuration.
