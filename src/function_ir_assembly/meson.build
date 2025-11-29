project('function_ir_assembly', 'cpp')

function_ir_dep = subproject('../function_ir').get_variable('function_ir_dep')

function_ir_assembly_lib = library(
  'function_ir_assembly',
  ['function_ir_assembly.cpp'],
  include_directories: include_directories('.'),
  dependencies: [function_ir_dep],
  install: false,
)

function_ir_assembly_dep = declare_dependency(
  include_directories: include_directories('.'),
  link_with: function_ir_assembly_lib,
)
