include("${CMAKE_CURRENT_LIST_DIR}/function.cmake")

add_library(function_alu OBJECT
    "${CMAKE_CURRENT_LIST_DIR}/function_alu.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/function_alu.h"
)
target_include_directories(function_alu PUBLIC "${CMAKE_CURRENT_LIST_DIR}")
target_compile_features(function_alu PUBLIC cxx_std_23)
target_link_libraries(function_alu PUBLIC function)
