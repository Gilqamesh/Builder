# Builder

Builder is a coding playground for experimenting with any kind of program by composing building blocks you place in `src/`. Each block is regular code you author; it can pull in other pieces from the same directory or operate alone. Entrypoints (tests, visualizers, etc.) live alongside those blocks in the same `src/` tree and are built by a single Meson project.

## What you can do
- Add new building blocks under `src/` that provide whatever functionality you need, reusing other files in that directory when helpful.
- Declare entrypoints in the corresponding `src/` package that list the exact blocks they depend on.
- Explore different ideas by changing which blocks an entrypoint pulls in while keeping each package self-contained.

## Repository layout
- `src/`: Building blocks and supporting code you author; each file lives in its own folder with a Meson build description that declares its sources and any sibling dependencies. External helper code that needs to be shared (e.g., imgui, rlImGui, ulid) also sits here in dedicated folders.

## Building with Meson
The repository uses a single Meson project. Configure it once, then build and test everything:

```bash
meson setup build -Dtests=enabled
meson compile -C build
meson test -C build
```

All internal dependencies are wired together through the top-level `src/meson.build`, so no wrap files or additional setup steps are required.

## Adding or modifying an entrypoint
1. Create a new folder under `src/` with its own build description.
2. Point that file at any external code it needs from elsewhere in `src/`, then list the exact sources you want to compile.
3. Declare only the blocks from `src/` your entrypoint depends on, and add new ones there as needed.
4. Add a helper script in `scripts/` if you want a one-liner to build or run the entrypoint.

This setup keeps entrypoints independent while letting you reuse or extend the code in `src/` however you choose.
