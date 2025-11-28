# Programs layout

Each program lives in its own subdirectory under `programs/` with a dedicated `CMakeLists.txt` that declares its targets and third-party dependencies. Third-party sources live beside each program (for example, `programs/visualizer/external`) so every program declares and owns the externals it needs. The layout keeps programs plug-compatible, letting each target pick the builder modules it needs without relying on a global external directory.

## Adding a new program
1. Create a directory under `programs/` (for example, `programs/my_app/`).
2. Add a `CMakeLists.txt` inside that directory. Point at the repository root and the program-local external folder, then list the exact sources you want to compile. A minimal pattern:
   ```cmake
   cmake_minimum_required(VERSION 3.14)
   project(my_app)

   get_filename_component(BUILDER_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
   set(MY_APP_EXTERNAL_DIR "${CMAKE_CURRENT_LIST_DIR}/external")

   set(MY_APP_SOURCES
       ${BUILDER_ROOT}/builder/call.cpp
       # add any other files your program needs
   )

   add_executable(my_app ${MY_APP_SOURCES})
   target_compile_features(my_app PRIVATE cxx_std_23)
   target_include_directories(my_app PRIVATE ${BUILDER_ROOT}/builder ${MY_APP_EXTERNAL_DIR})
   ```
3. Add a helper script in `scripts/` if you want a one-liner to build/run the new program.

Existing programs follow this layout to keep build definitions self-contained alongside their external dependencies. A shared `programs/external` directory is unnecessary because each program keeps the externals it needs next to its own sources.
