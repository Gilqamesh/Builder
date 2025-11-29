project('rlImGui', 'cpp', default_options: ['cpp_std=c++20'])

inc = include_directories('.', 'extras')
imgui_dep = dependency('imgui', fallback: ['imgui', 'imgui_dep'])
raylib_dep = dependency('raylib')

rlimgui_lib = library('rlimgui', ['rlImGui.cpp'],
  include_directories: inc,
  dependencies: [imgui_dep, raylib_dep],
  install: false)
rlimgui_dep = declare_dependency(link_with: rlimgui_lib, include_directories: inc, dependencies: [imgui_dep, raylib_dep])
