project('function', 'cpp')

function_ir_dep = subproject('../function_ir').get_variable('function_ir_dep')
typesystem_dep = subproject('../typesystem').get_variable('typesystem_dep')

function_lib = library(
  'function',
  ['function.cpp'],
  include_directories: include_directories('.'),
  dependencies: [function_ir_dep, typesystem_dep],
)

function_dep = declare_dependency(
  include_directories: include_directories('.'),
  link_with: function_lib,
)
