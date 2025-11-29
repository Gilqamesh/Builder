project('function_visualizer', 'cpp')

inc = include_directories('.')

rlimgui_dep = dependency('rlImGui')
imgui_dep = dependency('imgui')
function_alu_dep = dependency('function_alu')
function_compound_dep = dependency('function_compound')
function_ir_file_repository_dep = dependency('function_ir_file_repository')
function_repository_dep = dependency('function_repository')
function_visualizer_editor_dep = dependency('function_visualizer_editor')

function_visualizer_lib = library(
  'function_visualizer_lib',
  ['function_visualizer.cpp'],
  include_directories: inc,
  dependencies: [
    rlimgui_dep,
    imgui_dep,
    function_alu_dep,
    function_compound_dep,
    function_ir_file_repository_dep,
    function_repository_dep,
    function_visualizer_editor_dep,
  ],
  install: false,
)

executable(
  'function_visualizer',
  [],
  objects: function_visualizer_lib.extract_all_objects(),
  dependencies: [
    rlimgui_dep,
    imgui_dep,
    function_alu_dep,
    function_compound_dep,
    function_ir_file_repository_dep,
    function_repository_dep,
    function_visualizer_editor_dep,
  ],
)
