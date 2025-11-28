add_library(function_id OBJECT
    "${CMAKE_CURRENT_LIST_DIR}/function_id.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/function_id.h"
)
target_include_directories(function_id PUBLIC "${CMAKE_CURRENT_LIST_DIR}")
target_compile_features(function_id PUBLIC cxx_std_23)
