add_library(call OBJECT
    "${CMAKE_CURRENT_LIST_DIR}/call.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/call.h"
)
target_include_directories(call PUBLIC "${CMAKE_CURRENT_LIST_DIR}")
target_compile_features(call PUBLIC cxx_std_23)
