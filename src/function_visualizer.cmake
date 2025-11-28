include("${CMAKE_CURRENT_LIST_DIR}/function_alu.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/function_compound.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/function_ir_file_repository.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/function_repository.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/function_visualizer_editor.cmake")

file(GLOB_RECURSE FUNCTION_VISUALIZER_EXTERNAL_SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/function_visualizer_external/*.cpp"
)

include(FetchContent)
FetchContent_Declare(
  raylib
  GIT_REPOSITORY https://github.com/raysan5/raylib.git
  GIT_TAG 5.5
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(raylib)

add_executable(function_visualizer
    "${CMAKE_CURRENT_LIST_DIR}/function_visualizer.cpp"
    ${FUNCTION_VISUALIZER_EXTERNAL_SOURCES}
)

target_include_directories(function_visualizer PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}"
    "${CMAKE_CURRENT_LIST_DIR}/function_visualizer_external"
    "${CMAKE_CURRENT_LIST_DIR}/function_visualizer_external/imgui"
    "${CMAKE_CURRENT_LIST_DIR}/function_visualizer_external/imgui/backends"
    "${CMAKE_CURRENT_LIST_DIR}/function_visualizer_external/rlImGui"
)
target_compile_features(function_visualizer PRIVATE cxx_std_23)
target_link_libraries(function_visualizer PRIVATE
    function_alu
    function_compound
    function_ir_file_repository
    function_repository
    function_visualizer_editor
    raylib
)
set_target_properties(function_visualizer PROPERTIES LINK_FLAGS_RELEASE -s)
