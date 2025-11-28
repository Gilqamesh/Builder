cmake_minimum_required(VERSION 3.14)
project(builder_tests)

get_filename_component(BUILDER_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
set(BUILDER_EXTERNAL_DIR "${BUILDER_ROOT}/programs/external")

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

set(TEST_MAIN ${BUILDER_ROOT}/builder/tests_main.cpp)

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/heads/main.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(builder_tests ${BUILDER_CORE_SOURCES} ${TEST_MAIN})
target_compile_features(builder_tests PRIVATE cxx_std_23)
target_compile_definitions(builder_tests PRIVATE BUILDER_ENABLE_TESTS)
target_include_directories(builder_tests PRIVATE ${BUILDER_ROOT}/builder ${BUILDER_EXTERNAL_DIR})
target_link_libraries(builder_tests PRIVATE GTest::gtest_main)

enable_testing()
if(NOT CMAKE_CROSSCOMPILING)
    include(GoogleTest)
    gtest_discover_tests(builder_tests)
endif()
