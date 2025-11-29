cmake_minimum_required(VERSION 3.14)
project(tests)

get_filename_component(SRC_DIR "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

set(SRC
    ${SRC_DIR}/call/call.cpp
    ${SRC_DIR}/call/call_test.cpp
    ${SRC_DIR}/function/function.cpp
    ${SRC_DIR}/function/function_test.cpp
    ${SRC_DIR}/function_alu/function_alu.cpp
    ${SRC_DIR}/function_alu/function_alu_test.cpp
    ${SRC_DIR}/function_call_repository/function_call_repository.cpp
    ${SRC_DIR}/function_call_repository/function_call_repository_test.cpp
    ${SRC_DIR}/function_compound/function_compound.cpp
    ${SRC_DIR}/function_compound/function_compound_test.cpp
    ${SRC_DIR}/function_id/function_id.cpp
    ${SRC_DIR}/function_id/function_id_test.cpp
    ${SRC_DIR}/function_ir/function_ir.cpp
    ${SRC_DIR}/function_ir/function_ir_test.cpp
    ${SRC_DIR}/function_ir_assembly/function_ir_assembly.cpp
    ${SRC_DIR}/function_ir_assembly/function_ir_assembly_test.cpp
    ${SRC_DIR}/function_ir_binary/function_ir_binary.cpp
    ${SRC_DIR}/function_ir_binary/function_ir_binary_test.cpp
    ${SRC_DIR}/function_ir_file_repository/function_ir_file_repository.cpp
    ${SRC_DIR}/function_ir_file_repository/function_ir_file_repository_test.cpp
    ${SRC_DIR}/function_primitive_cpp/function_primitive_cpp.cpp
    ${SRC_DIR}/function_primitive_cpp/function_primitive_cpp_test.cpp
    ${SRC_DIR}/function_primitive_lang/function_primitive_lang.cpp
    ${SRC_DIR}/function_primitive_lang/function_primitive_lang_test.cpp
    ${SRC_DIR}/function_repository/function_repository.cpp
    ${SRC_DIR}/function_repository/function_repository_test.cpp
    ${SRC_DIR}/typesystem/typesystem.cpp
    ${SRC_DIR}/typesystem/typesystem_test.cpp
    ${SRC_DIR}/typesystem_call/typesystem_call.cpp
    ${SRC_DIR}/typesystem_call/typesystem_call_test.cpp
    ${SRC_DIR}/tests_main/tests_main.cpp
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
)

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(tests ${SRC})
target_compile_features(tests PRIVATE cxx_std_23)
target_include_directories(tests PRIVATE ${INCLUDE_DIRS})
target_link_libraries(tests PRIVATE GTest::gtest_main)

enable_testing()
if(NOT CMAKE_CROSSCOMPILING)
    include(GoogleTest)
    gtest_discover_tests(tests)
endif()
