file(GLOB SRC *.cpp)
add_executable(builder_app ${SRC})
target_link_libraries(builder_app PRIVATE builder_lib)

if(NOT MSVC)
target_link_options(builder_app PRIVATE -static-libgcc -static-libstdc++)
endif()

set_target_properties(builder_app PROPERTIES LINK_FLAGS_RELEASE -s)
