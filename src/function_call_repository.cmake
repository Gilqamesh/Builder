include("${CMAKE_CURRENT_LIST_DIR}/function.cmake")

add_library(function_call_repository OBJECT
    "${CMAKE_CURRENT_LIST_DIR}/function_call_repository.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/function_call_repository.h"
)
target_include_directories(function_call_repository PUBLIC "${CMAKE_CURRENT_LIST_DIR}")
target_compile_features(function_call_repository PUBLIC cxx_std_23)
target_link_libraries(function_call_repository PUBLIC function)
