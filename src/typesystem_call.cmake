include("${CMAKE_CURRENT_LIST_DIR}/typesystem.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/call.cmake")

add_library(typesystem_call OBJECT
    "${CMAKE_CURRENT_LIST_DIR}/typesystem_call.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/typesystem_call.h"
)
target_include_directories(typesystem_call PUBLIC "${CMAKE_CURRENT_LIST_DIR}")
target_compile_features(typesystem_call PUBLIC cxx_std_23)
target_link_libraries(typesystem_call PUBLIC typesystem call)
