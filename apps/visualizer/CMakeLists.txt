include(FetchContent)
FetchContent_Declare(
  raylib
  GIT_REPOSITORY https://github.com/raysan5/raylib.git
  GIT_TAG 5.5
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(raylib)

file(GLOB_RECURSE SRC *.cpp)
add_executable(visualizer ${SRC})
target_link_libraries(visualizer PRIVATE builder_lib raylib)
set_target_properties(visualizer PROPERTIES LINK_FLAGS_RELEASE -s)
