include("${CMAKE_CURRENT_LIST_DIR}/function_ir.cmake")

add_library(function_ir_assembly OBJECT
    "${CMAKE_CURRENT_LIST_DIR}/function_ir_assembly.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/function_ir_assembly.h"
)
target_include_directories(function_ir_assembly PUBLIC "${CMAKE_CURRENT_LIST_DIR}")
target_compile_features(function_ir_assembly PUBLIC cxx_std_23)
target_link_libraries(function_ir_assembly PUBLIC function_ir)
