project('function_visualizer_editor', 'cpp')

imgui_dep = subproject('../imgui').get_variable('imgui_dep')
rlimgui_dep = subproject('../rlImGui').get_variable('rlImGui_dep')

function_visualizer_editor_lib = library(
  'function_visualizer_editor',
  ['function_visualizer_editor.cpp'],
  include_directories: include_directories('.'),
  dependencies: [imgui_dep, rlimgui_dep],
  install: false,
)

function_visualizer_editor_dep = declare_dependency(
  include_directories: include_directories('.'),
  link_with: function_visualizer_editor_lib,
)
