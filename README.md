# Builder

Builder is a coding playground for experimenting with any kind of program by composing building blocks you place in `src/`. Each block is regular code you author; it can pull in other pieces from the same directory or operate alone. Programs sit in `programs/` as entrypoints that declare the blocks they rely on and any external code they carry with them.

## What you can do
- Add new building blocks under `src/` that provide whatever functionality you need, reusing other files in that directory when helpful.
- Create programs under `programs/` that list the exact blocks they depend on and keep any externals local to that entrypoint.
- Explore different ideas by changing which blocks a program pulls in while keeping each program self-contained.

## Repository layout
- `src/`: Building blocks and supporting code you author; each file can depend on other files in this directory.
- `programs/`: Entrypoints that describe how to build a particular idea and which blocks it uses, along with any program-specific external dependencies.
- `scripts/`: Convenience wrappers that sit on top of `programs/` to make building a given entrypoint easy.

## Building
Use the script that matches the program you want to compile. Each script creates its own build directory under `build/`.

```bash
# Build a program (examples exist under scripts/)
./scripts/build_tests.sh
```

## Adding or modifying a program
1. Create a new folder under `programs/` with its own build description.
2. Point that file at the repository root and a program-local `external/` directory, then list the exact sources you want to compile.
3. Declare only the blocks from `src/` your program depends on, and add new ones there as needed.
4. Add a helper script in `scripts/` if you want a one-liner to build or run the program.

This setup keeps programs independent while letting you reuse or extend the code in `src/` however you choose.
