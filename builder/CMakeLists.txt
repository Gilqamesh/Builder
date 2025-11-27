file(GLOB_RECURSE SRC CONFIGURE_DEPENDS *.cpp)
list(FILTER SRC EXCLUDE REGEX ".*/tests/.*")
list(FILTER SRC EXCLUDE REGEX ".*/visualizer_.*\\.cpp$")

add_library(builder_lib ${SRC})

target_include_directories(builder_lib PRIVATE external)
target_include_directories(builder_lib PUBLIC .)

# target_compile_options(builder_lib PUBLIC -Wall -Wextra -Werror -pedantic -Wfloat-equal)

target_link_libraries(builder_lib PUBLIC project_interface)

if(ENABLE_TESTS)
    add_subdirectory(tests)
endif()

if(ENABLE_VISUALIZER)
    include(FetchContent)
    FetchContent_Declare(
      raylib
      GIT_REPOSITORY https://github.com/raysan5/raylib.git
      GIT_TAG 5.5
      GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(raylib)

    set(VISUALIZER_SOURCES
        visualizer_app.cpp
        visualizer_editor.cpp
        visualizer_interaction.cpp
        visualizer_render.cpp
        visualizer_space.cpp
        visualizer_state.cpp
    )

    add_executable(visualizer ${VISUALIZER_SOURCES})
    target_link_libraries(visualizer PRIVATE builder_lib raylib)
    target_include_directories(visualizer PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/external/visualizer/third_party/imgui
        ${CMAKE_CURRENT_LIST_DIR}/external/visualizer/third_party/rlImGui
    )
    set_target_properties(visualizer PROPERTIES LINK_FLAGS_RELEASE -s)
endif()
