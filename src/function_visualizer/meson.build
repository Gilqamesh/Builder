project('function_visualizer', 'cpp', default_options: ['cpp_std=c++20'])

inc = include_directories('.')

rlimgui_dep = dependency('rlImGui', fallback: ['rlImGui', 'rlimgui_dep'])
imgui_dep = dependency('imgui', fallback: ['imgui', 'imgui_dep'])
function_alu_dep = dependency('function_alu', fallback: ['function_alu', 'function_alu_dep'])
function_compound_dep = dependency('function_compound', fallback: ['function_compound', 'function_compound_dep'])
function_ir_file_repository_dep = dependency('function_ir_file_repository', fallback: ['function_ir_file_repository', 'function_ir_file_repository_dep'])
function_repository_dep = dependency('function_repository', fallback: ['function_repository', 'function_repository_dep'])
function_visualizer_editor_dep = dependency('function_visualizer_editor', fallback: ['function_visualizer_editor', 'function_visualizer_editor_dep'])

function_visualizer_lib = library('function_visualizer_lib', ['function_visualizer.cpp'],
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
  install: false)

executable('function_visualizer', [],
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
