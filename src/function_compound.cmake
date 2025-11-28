include("${CMAKE_CURRENT_LIST_DIR}/function.cmake")

add_library(function_compound OBJECT
    "${CMAKE_CURRENT_LIST_DIR}/function_compound.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/function_compound.h"
)
target_include_directories(function_compound PUBLIC "${CMAKE_CURRENT_LIST_DIR}")
target_compile_features(function_compound PUBLIC cxx_std_23)
target_link_libraries(function_compound PUBLIC function)
