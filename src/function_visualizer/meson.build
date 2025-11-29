project('function_visualizer', 'cpp')

rlimgui_dep = subproject('../rlImGui').get_variable('rlImGui_dep')
imgui_dep = subproject('../imgui').get_variable('imgui_dep')
function_alu_dep = subproject('../function_alu').get_variable('function_alu_dep')
function_compound_dep = subproject('../function_compound').get_variable('function_compound_dep')
function_ir_file_repository_dep = subproject('../function_ir_file_repository').get_variable('function_ir_file_repository_dep')
function_repository_dep = subproject('../function_repository').get_variable('function_repository_dep')
function_visualizer_editor_dep = subproject('../function_visualizer_editor').get_variable('function_visualizer_editor_dep')

function_visualizer_lib = library(
  'function_visualizer_lib',
  ['function_visualizer.cpp'],
  include_directories: include_directories('.'),
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

function_visualizer_dep = declare_dependency(
  include_directories: include_directories('.'),
  link_with: function_visualizer_lib,
)

executable(
  'function_visualizer',
  [],
  objects: function_visualizer_lib.extract_all_objects(),
  dependencies: [
    function_visualizer_dep,
    rlimgui_dep,
    imgui_dep,
    function_alu_dep,
    function_compound_dep,
    function_ir_file_repository_dep,
    function_repository_dep,
    function_visualizer_editor_dep,
  ],
)
