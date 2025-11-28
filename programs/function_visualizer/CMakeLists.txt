cmake_minimum_required(VERSION 3.14)
project(builder_visualizer)

get_filename_component(SRC_DIR "${CMAKE_CURRENT_LIST_DIR}/../../src" ABSOLUTE)
set(SRC
    ${SRC_DIR}/call.cpp
    ${SRC_DIR}/function.cpp
    ${SRC_DIR}/function_alu.cpp
    ${SRC_DIR}/function_call_repository.cpp
    ${SRC_DIR}/function_compound.cpp
    ${SRC_DIR}/function_id.cpp
    ${SRC_DIR}/function_ir.cpp
    ${SRC_DIR}/function_ir_assembly.cpp
    ${SRC_DIR}/function_ir_binary.cpp
    ${SRC_DIR}/function_ir_file_repository.cpp
    ${SRC_DIR}/function_primitive_cpp.cpp
    ${SRC_DIR}/function_primitive_lang.cpp
    ${SRC_DIR}/function_repository.cpp
    ${SRC_DIR}/typesystem.cpp
    ${SRC_DIR}/typesystem_call.cpp
    ${SRC_DIR}/function_visualizer.cpp
    ${SRC_DIR}/function_visualizer_editor.cpp
)

set(EXTERNAL_SRC_DIR "${CMAKE_CURRENT_LIST_DIR}/external")
file(GLOB_RECURSE EXTERNAL_SRC "${EXTERNAL_SRC_DIR}/*.cpp")

include(FetchContent)
FetchContent_Declare(
  raylib
  GIT_REPOSITORY https://github.com/raysan5/raylib.git
  GIT_TAG 5.5
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(raylib)

add_executable(function_visualizer ${SRC} ${EXTERNAL_SRC})

target_compile_features(function_visualizer PRIVATE cxx_std_23)
target_include_directories(function_visualizer PRIVATE
    ${SRC_DIR}
    ${EXTERNAL_SRC_DIR}
    ${EXTERNAL_SRC_DIR}/imgui
    ${EXTERNAL_SRC_DIR}/imgui/backends
    ${EXTERNAL_SRC_DIR}/rlImGui
)
target_link_libraries(function_visualizer PRIVATE raylib)
set_target_properties(function_visualizer PROPERTIES LINK_FLAGS_RELEASE -s)
