cmake_minimum_required(VERSION 3.14)
project(tests)

get_filename_component(SRC_DIR "${CMAKE_CURRENT_LIST_DIR}/../../src" ABSOLUTE)
set(EXTERNAL_DIR "${CMAKE_CURRENT_LIST_DIR}/external")

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
    ${SRC_DIR}/tests_main.cpp
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
target_compile_definitions(tests PRIVATE BUILDER_ENABLE_TESTS)
target_include_directories(tests PRIVATE
    ${SRC_DIR}
    ${EXTERNAL_DIR}
    ${EXTERNAL_DIR}/ulid
)
target_link_libraries(tests PRIVATE GTest::gtest_main)

enable_testing()
if(NOT CMAKE_CROSSCOMPILING)
    include(GoogleTest)
    gtest_discover_tests(tests)
endif()
