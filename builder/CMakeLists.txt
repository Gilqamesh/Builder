file(GLOB_RECURSE SRC *.cpp)

add_library(builder_lib ${SRC})

target_include_directories(builder_lib PRIAVTE external)

target_link_libraries(builder_lib PRIVATE project_interface)

if(ENABLE_TESTS)
    add_subdirectory(tests)
endif()
