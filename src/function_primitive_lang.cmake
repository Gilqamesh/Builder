include("${CMAKE_CURRENT_LIST_DIR}/function.cmake")

add_library(function_primitive_lang OBJECT
    "${CMAKE_CURRENT_LIST_DIR}/function_primitive_lang.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/function_primitive_lang.h"
)
target_include_directories(function_primitive_lang PUBLIC "${CMAKE_CURRENT_LIST_DIR}")
target_compile_features(function_primitive_lang PUBLIC cxx_std_23)
target_link_libraries(function_primitive_lang PUBLIC function)
