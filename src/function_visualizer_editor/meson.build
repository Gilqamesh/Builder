inc = include_directories('.')

imgui_dep = dependency('imgui')
rlimgui_dep = dependency('rlImGui')

function_visualizer_editor_lib = library(
  'function_visualizer_editor',
  ['function_visualizer_editor.cpp'],
  include_directories: inc,
  dependencies: [imgui_dep, rlimgui_dep],
  install: false,
)
function_visualizer_editor_dep = declare_dependency(
  link_with: function_visualizer_editor_lib,
  include_directories: inc,
  dependencies: [imgui_dep, rlimgui_dep],
)
meson.override_dependency('function_visualizer_editor', function_visualizer_editor_dep)
