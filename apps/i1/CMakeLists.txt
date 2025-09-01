file(GLOB SRC *.cpp)
add_executable(i1 ${SRC})
target_link_libraries(i1 PRIVATE builder_lib)

if(NOT MSVC)
target_link_options(i1 PRIVATE -static-libgcc -static-libstdc++)
endif()

set_target_properties(i1 PROPERTIES LINK_FLAGS_RELEASE -s)
