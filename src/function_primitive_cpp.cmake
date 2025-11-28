include("${CMAKE_CURRENT_LIST_DIR}/function.cmake")

add_library(function_primitive_cpp OBJECT
    "${CMAKE_CURRENT_LIST_DIR}/function_primitive_cpp.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/function_primitive_cpp.h"
)
target_include_directories(function_primitive_cpp PUBLIC "${CMAKE_CURRENT_LIST_DIR}")
target_compile_features(function_primitive_cpp PUBLIC cxx_std_23)
target_link_libraries(function_primitive_cpp PUBLIC function)
