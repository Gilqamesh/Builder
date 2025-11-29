inc = include_directories('.')

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
