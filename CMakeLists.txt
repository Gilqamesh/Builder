cmake_minimum_required(VERSION 3.14)

project(builder)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

enable_testing()

option(ENABLE_VISUALIZER "Build visualizer application" OFF)

add_library(project_interface INTERFACE)
target_include_directories(project_interface INTERFACE ${CMAKE_SOURCE_DIR}/include)
target_precompile_headers(project_interface INTERFACE ${CMAKE_SOURCE_DIR}/include/libc.h)
target_link_libraries(project_interface INTERFACE m)

add_subdirectory(builder)
