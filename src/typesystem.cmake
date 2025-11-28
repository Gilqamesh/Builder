add_library(typesystem OBJECT
    "${CMAKE_CURRENT_LIST_DIR}/typesystem.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/typesystem.h"
)
target_include_directories(typesystem PUBLIC "${CMAKE_CURRENT_LIST_DIR}")
target_compile_features(typesystem PUBLIC cxx_std_23)
