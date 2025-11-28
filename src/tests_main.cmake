include("${CMAKE_CURRENT_LIST_DIR}/call.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/function.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/function_alu.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/function_call_repository.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/function_compound.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/function_id.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/function_ir.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/function_ir_assembly.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/function_ir_binary.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/function_ir_file_repository.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/function_primitive_cpp.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/function_primitive_lang.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/function_repository.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/typesystem.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/typesystem_call.cmake")

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(googletest)

add_executable(tests "${CMAKE_CURRENT_LIST_DIR}/tests_main.cpp")

target_link_libraries(tests PRIVATE
    call
    function
    function_alu
    function_call_repository
    function_compound
    function_id
    function_ir
    function_ir_assembly
    function_ir_binary
    function_ir_file_repository
    function_primitive_cpp
    function_primitive_lang
    function_repository
    typesystem
    typesystem_call
    GTest::gtest_main
)

target_compile_features(tests PRIVATE cxx_std_23)
target_compile_definitions(tests PRIVATE BUILDER_ENABLE_TESTS)
target_include_directories(tests PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}"
    "${CMAKE_CURRENT_LIST_DIR}/tests_external"
)

enable_testing()
if(NOT CMAKE_CROSSCOMPILING)
    include(GoogleTest)
    gtest_discover_tests(tests)
endif()
