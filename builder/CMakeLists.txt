file(GLOB_RECURSE SRC *.cpp)

add_library(builder_lib ${SRC})

target_precompile_headers(builder_lib PRIVATE core/libc.h)

target_include_directories(builder_lib PUBLIC .)

if (MSVC)
    target_compile_options(builder_lib PRIVATE /W4 /WX /permissive-)
else()
    target_compile_options(builder_lib PRIVATE -Wall -Wextra -pedantic -Werror -Wfloat-equal)
    target_link_libraries(builder_lib PUBLIC m)
endif()

if(ENABLE_TESTS)
    add_subdirectory(tests)
endif()
