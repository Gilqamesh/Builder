include("${CMAKE_CURRENT_LIST_DIR}/function_ir.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/typesystem.cmake")

add_library(function OBJECT
    "${CMAKE_CURRENT_LIST_DIR}/function.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/function.h"
)
target_include_directories(function PUBLIC "${CMAKE_CURRENT_LIST_DIR}")
target_compile_features(function PUBLIC cxx_std_23)
target_link_libraries(function PUBLIC function_ir typesystem)
