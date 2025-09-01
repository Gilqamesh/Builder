file(GLOB SRC *.cpp)
add_executable(i2 ${SRC})
target_link_libraries(i2 PRIVATE builder_lib)

if(NOT MSVC)
target_link_options(i2 PRIVATE -static-libgcc -static-libstdc++)
endif()

set_target_properties(i2 PROPERTIES LINK_FLAGS_RELEASE -s)
