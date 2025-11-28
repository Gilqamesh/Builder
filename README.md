# Builder

Builder is a C++ playground for experimenting with any kind of program by composing building blocks you place in `src/`. Each block is regular code you author; it can pull in other pieces from the same directory or operate alone, and every block ships with its own CMake fragment so you can include it directly from another project.

## What you can do
- Add new building blocks under `src/` that provide whatever functionality you need, reusing other files in that directory when helpful.
- Include the CMake file for a block from another project to reuse it without any shared build scaffolding.
- Explore different ideas by changing which blocks a consumer pulls in while keeping each block self-contained.

## Repository layout
- `src/`: Building blocks, their companion CMake files, and any external code they rely on.
- `scripts/`: Convenience wrappers that generate a tiny CMake project for building a specific block (e.g., tests or the function visualizer).

## Building
Use the script that matches the block you want to compile. Each script creates its own build directory under `build/` and points CMake at the corresponding module file under `src/`.

```bash
# Build a block (examples exist under scripts/)
./scripts/build_tests.sh
```
## Adding or modifying a block
1. Add your new source and header files under `src/`.
2. Create a matching `.cmake` file next to them that declares the target, its include path, and any other module CMake files it depends on.
3. If you want a quick way to build or run the block from this repository, add a helper script under `scripts/` that points CMake at your new module file.
