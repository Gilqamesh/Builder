project('rlImGui', 'cpp')

inc = include_directories('.', 'extras')

imgui_dep = subproject('../imgui').get_variable('imgui_dep')
raylib_dep = dependency('raylib', required: true)

rlimgui_lib = library(
  'rlimgui',
  ['rlImGui.cpp'],
  include_directories: inc,
  dependencies: [imgui_dep, raylib_dep],
  install: false,
)

rlImGui_dep = declare_dependency(
  include_directories: inc,
  link_with: rlimgui_lib,
)
