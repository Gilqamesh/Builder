project('imgui', 'cpp', default_options: ['cpp_std=c++20'])

inc = include_directories('.', 'backends', 'misc')

imgui_sources = [
  'imgui.cpp',
  'imgui_demo.cpp',
  'imgui_draw.cpp',
  'imgui_tables.cpp',
  'imgui_widgets.cpp',
]

imgui_lib = library('imgui', imgui_sources, include_directories: inc, install: false)
imgui_dep = declare_dependency(link_with: imgui_lib, include_directories: inc)
