project('function_ir_file_repository', 'cpp')

function_ir_dep = subproject('../function_ir').get_variable('function_ir_dep')
function_ir_binary_dep = subproject('../function_ir_binary').get_variable('function_ir_binary_dep')
typesystem_dep = subproject('../typesystem').get_variable('typesystem_dep')

function_ir_file_repository_lib = library(
  'function_ir_file_repository',
  ['function_ir_file_repository.cpp'],
  include_directories: include_directories('.'),
  dependencies: [function_ir_dep, function_ir_binary_dep, typesystem_dep],
  install: false,
)

function_ir_file_repository_dep = declare_dependency(
  include_directories: include_directories('.'),
  link_with: function_ir_file_repository_lib,
)
