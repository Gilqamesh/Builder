project(builder_library)

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

add_library(builder_lib ${BUILDER_CORE_SOURCES})
target_compile_features(builder_lib PUBLIC cxx_std_23)
target_include_directories(builder_lib PRIVATE ${BUILDER_ROOT}/builder/external)
target_include_directories(builder_lib PUBLIC ${BUILDER_ROOT}/builder)
