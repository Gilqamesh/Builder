include("${CMAKE_CURRENT_LIST_DIR}/function_ir.cmake")

add_library(function_ir_binary OBJECT
    "${CMAKE_CURRENT_LIST_DIR}/function_ir_binary.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/function_ir_binary.h"
)
target_include_directories(function_ir_binary PUBLIC "${CMAKE_CURRENT_LIST_DIR}")
target_compile_features(function_ir_binary PUBLIC cxx_std_23)
target_link_libraries(function_ir_binary PUBLIC function_ir)
