add_library(function_visualizer_editor OBJECT
    "${CMAKE_CURRENT_LIST_DIR}/function_visualizer_editor.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/function_visualizer_editor.h"
)
target_include_directories(function_visualizer_editor PUBLIC "${CMAKE_CURRENT_LIST_DIR}")
target_compile_features(function_visualizer_editor PUBLIC cxx_std_23)
