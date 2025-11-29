inc = include_directories('.', 'extras')
raylib_dep = dependency('raylib')

rlimgui_lib = library('rlimgui', ['rlImGui.cpp'],
  include_directories: inc,
  dependencies: [imgui_dep, raylib_dep],
  install: false)
rlimgui_dep = declare_dependency(link_with: rlimgui_lib, include_directories: inc, dependencies: [imgui_dep, raylib_dep])
