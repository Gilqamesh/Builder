project('function_ir_assembly', 'cpp')

inc = include_directories('.')

function_ir_dep = dependency('function_ir')

function_ir_assembly_lib = library(
  'function_ir_assembly',
  ['function_ir_assembly.cpp'],
  include_directories: inc,
  dependencies: [function_ir_dep],
  install: false,
)
function_ir_assembly_dep = declare_dependency(
  link_with: function_ir_assembly_lib,
  include_directories: inc,
  dependencies: [function_ir_dep],
)
meson.override_dependency('function_ir_assembly', function_ir_assembly_dep)
