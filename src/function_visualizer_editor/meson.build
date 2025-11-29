project('function_visualizer_editor', 'cpp', default_options: ['cpp_std=c++20'])

inc = include_directories('.')
imgui_dep = dependency('imgui', fallback: ['imgui', 'imgui_dep'])
rlimgui_dep = dependency('rlImGui', fallback: ['rlImGui', 'rlimgui_dep'])

function_visualizer_editor_lib = library('function_visualizer_editor', ['function_visualizer_editor.cpp'],
  include_directories: inc,
  dependencies: [imgui_dep, rlimgui_dep],
  install: false)
function_visualizer_editor_dep = declare_dependency(
  link_with: function_visualizer_editor_lib,
  include_directories: inc,
  dependencies: [imgui_dep, rlimgui_dep],
)
