file(GLOB SRC *.cpp)
add_executable(i4 ${SRC})
target_link_libraries(i4 PRIVATE builder_lib)

if(NOT MSVC)
target_link_options(i4 PRIVATE -static-libgcc -static-libstdc++)
endif()

set_target_properties(i4 PROPERTIES LINK_FLAGS_RELEASE -s)
