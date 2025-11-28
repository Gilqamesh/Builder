project(builder_visualizer)

get_filename_component(BUILDER_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

set(BUILDER_CORE_SOURCES
    ${BUILDER_ROOT}/builder/call.cpp
    ${BUILDER_ROOT}/builder/function.cpp
    ${BUILDER_ROOT}/builder/function_alu.cpp
    ${BUILDER_ROOT}/builder/function_call_repository.cpp
    ${BUILDER_ROOT}/builder/function_compound.cpp
    ${BUILDER_ROOT}/builder/function_id.cpp
    ${BUILDER_ROOT}/builder/function_ir.cpp
    ${BUILDER_ROOT}/builder/function_ir_assembly.cpp
    ${BUILDER_ROOT}/builder/function_ir_binary.cpp
    ${BUILDER_ROOT}/builder/function_ir_file_repository.cpp
    ${BUILDER_ROOT}/builder/function_primitive_cpp.cpp
    ${BUILDER_ROOT}/builder/function_primitive_lang.cpp
    ${BUILDER_ROOT}/builder/function_repository.cpp
    ${BUILDER_ROOT}/builder/typesystem.cpp
    ${BUILDER_ROOT}/builder/typesystem_call.cpp
)

set(VISUALIZER_SOURCES
    ${BUILDER_ROOT}/builder/visualizer_app.cpp
    ${BUILDER_ROOT}/builder/visualizer_editor.cpp
    ${BUILDER_ROOT}/builder/visualizer_interaction.cpp
    ${BUILDER_ROOT}/builder/visualizer_render.cpp
    ${BUILDER_ROOT}/builder/visualizer_space.cpp
    ${BUILDER_ROOT}/builder/visualizer_state.cpp
)

include(FetchContent)
FetchContent_Declare(
  raylib
  GIT_REPOSITORY https://github.com/raysan5/raylib.git
  GIT_TAG 5.5
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(raylib)

add_executable(visualizer ${BUILDER_CORE_SOURCES} ${VISUALIZER_SOURCES})
target_compile_features(visualizer PRIVATE cxx_std_23)
target_include_directories(visualizer PRIVATE
    ${BUILDER_ROOT}/builder
    ${BUILDER_ROOT}/builder/external
    ${BUILDER_ROOT}/builder/external/visualizer/third_party/imgui
    ${BUILDER_ROOT}/builder/external/visualizer/third_party/rlImGui
)
target_link_libraries(visualizer PRIVATE raylib)
set_target_properties(visualizer PROPERTIES LINK_FLAGS_RELEASE -s)
