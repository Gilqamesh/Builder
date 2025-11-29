project('function_ir', 'cpp')

function_id_dep = subproject('../function_id').get_variable('function_id_dep')

function_ir_lib = library(
  'function_ir',
  ['function_ir.cpp'],
  include_directories: include_directories('.'),
  dependencies: [function_id_dep],
)

function_ir_dep = declare_dependency(
  include_directories: include_directories('.'),
  link_with: function_ir_lib,
)
