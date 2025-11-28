include("${CMAKE_CURRENT_LIST_DIR}/function_ir.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/typesystem.cmake")

add_library(function_ir_file_repository OBJECT
    "${CMAKE_CURRENT_LIST_DIR}/function_ir_file_repository.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/function_ir_file_repository.h"
)
target_include_directories(function_ir_file_repository PUBLIC "${CMAKE_CURRENT_LIST_DIR}")
target_compile_features(function_ir_file_repository PUBLIC cxx_std_23)
target_link_libraries(function_ir_file_repository PUBLIC function_ir typesystem)
