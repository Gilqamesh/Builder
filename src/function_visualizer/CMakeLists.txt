cmake_minimum_required(VERSION 3.14)
project(builder_visualizer)

get_filename_component(SRC_DIR "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

set(SRC
    ${SRC_DIR}/call/call.cpp
    ${SRC_DIR}/function/function.cpp
    ${SRC_DIR}/function_alu/function_alu.cpp
    ${SRC_DIR}/function_call_repository/function_call_repository.cpp
    ${SRC_DIR}/function_compound/function_compound.cpp
    ${SRC_DIR}/function_id/function_id.cpp
    ${SRC_DIR}/function_ir/function_ir.cpp
    ${SRC_DIR}/function_ir_assembly/function_ir_assembly.cpp
    ${SRC_DIR}/function_ir_binary/function_ir_binary.cpp
    ${SRC_DIR}/function_ir_file_repository/function_ir_file_repository.cpp
    ${SRC_DIR}/function_primitive_cpp/function_primitive_cpp.cpp
    ${SRC_DIR}/function_primitive_lang/function_primitive_lang.cpp
    ${SRC_DIR}/function_repository/function_repository.cpp
    ${SRC_DIR}/typesystem/typesystem.cpp
    ${SRC_DIR}/typesystem_call/typesystem_call.cpp
    ${SRC_DIR}/function_visualizer/function_visualizer.cpp
    ${SRC_DIR}/function_visualizer_editor/function_visualizer_editor.cpp
)

set(INCLUDE_DIRS
    ${SRC_DIR}/call
    ${SRC_DIR}/function
    ${SRC_DIR}/function_alu
    ${SRC_DIR}/function_call_repository
    ${SRC_DIR}/function_compound
    ${SRC_DIR}/function_id
    ${SRC_DIR}/function_ir
    ${SRC_DIR}/function_ir_assembly
    ${SRC_DIR}/function_ir_binary
    ${SRC_DIR}/function_ir_file_repository
    ${SRC_DIR}/function_primitive_cpp
    ${SRC_DIR}/function_primitive_lang
    ${SRC_DIR}/function_repository
    ${SRC_DIR}/typesystem
    ${SRC_DIR}/typesystem_call
    ${SRC_DIR}/function_visualizer
    ${SRC_DIR}/function_visualizer_editor
)

set(IMGUI_DIR ${SRC_DIR}/imgui)
file(GLOB IMGUI_SRC "${IMGUI_DIR}/*.cpp" "${IMGUI_DIR}/backends/*.cpp")
set(RLIMGUI_DIR ${SRC_DIR}/rlImGui)
file(GLOB RLIMGUI_SRC "${RLIMGUI_DIR}/*.cpp")
set(EXTERNAL_SRC ${IMGUI_SRC} ${RLIMGUI_SRC})

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
target_include_directories(function_visualizer PRIVATE ${INCLUDE_DIRS} ${IMGUI_DIR} ${IMGUI_DIR}/backends ${RLIMGUI_DIR})
target_link_libraries(function_visualizer PRIVATE raylib)
set_target_properties(function_visualizer PROPERTIES LINK_FLAGS_RELEASE -s)
