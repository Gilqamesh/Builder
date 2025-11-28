include("${CMAKE_CURRENT_LIST_DIR}/function_id.cmake")

add_library(function_ir OBJECT
    "${CMAKE_CURRENT_LIST_DIR}/function_ir.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/function_ir.h"
)
target_include_directories(function_ir PUBLIC "${CMAKE_CURRENT_LIST_DIR}")
target_compile_features(function_ir PUBLIC cxx_std_23)
target_link_libraries(function_ir PUBLIC function_id)
