file(GLOB SRC *.cpp)
add_executable(i3 ${SRC})
target_link_libraries(i3 PRIVATE builder_lib)

if(NOT MSVC)
target_link_options(i3 PRIVATE -static-libgcc -static-libstdc++)
endif()

set_target_properties(i3 PROPERTIES LINK_FLAGS_RELEASE -s)
