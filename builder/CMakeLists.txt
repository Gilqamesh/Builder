file(GLOB_RECURSE SRC CONFIGURE_DEPENDS *.cpp)
list(FILTER SRC EXCLUDE REGEX ".*/tests/.*")

add_library(builder_lib ${SRC})

target_include_directories(builder_lib PRIVATE external)
target_include_directories(builder_lib PUBLIC .)

# target_compile_options(builder_lib PUBLIC -Wall -Wextra -Werror -pedantic -Wfloat-equal)

target_link_libraries(builder_lib PUBLIC project_interface)

if(ENABLE_TESTS)
    add_subdirectory(tests)
endif()
