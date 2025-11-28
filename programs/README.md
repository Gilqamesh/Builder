# Programs layout

Program entrypoints are defined by standalone CMake files that live directly under `programs/` (for example, `library.cmake`, `tests.cmake`, and `visualizer.cmake`). Each file declares everything needed to build its program, including explicit source lists from `builder/`.

## Adding a new program
1. Create a new CMake file under `programs/` (for example, `programs/my_app.cmake`).
2. Declare the project, point at the repository root, and list the exact source files you want to compile. A minimal pattern:
   ```cmake
   project(my_app)

   get_filename_component(BUILDER_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

   set(MY_APP_SOURCES
       ${BUILDER_ROOT}/builder/call.cpp
       # add any other files your program needs
   )

   add_executable(my_app ${MY_APP_SOURCES})
   target_compile_features(my_app PRIVATE cxx_std_23)
   target_include_directories(my_app PRIVATE ${BUILDER_ROOT}/builder ${BUILDER_ROOT}/builder/external)
   ```
3. Add a helper script in `scripts/` if you want a one-liner to build/run the new program. Mirror the existing `build_*.sh` scripts so each entrypoint can be driven independently by passing `PROGRAM_FILE` to CMake.

Existing programs reuse this pattern while keeping every build definition colocated in the `programs/` directory.
