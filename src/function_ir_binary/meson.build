project('function_ir_binary', 'cpp')

function_ir_dep = subproject('../function_ir').get_variable('function_ir_dep')

function_ir_binary_lib = library(
  'function_ir_binary',
  ['function_ir_binary.cpp'],
  include_directories: include_directories('.'),
  dependencies: [function_ir_dep],
  install: false,
)

function_ir_binary_dep = declare_dependency(
  include_directories: include_directories('.'),
  link_with: function_ir_binary_lib,
)
