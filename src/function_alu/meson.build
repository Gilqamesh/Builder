project('function_alu', 'cpp')

function_dep = subproject('../function').get_variable('function_dep')
typesystem_dep = subproject('../typesystem').get_variable('typesystem_dep')

function_alu_lib = library(
  'function_alu',
  ['function_alu.cpp'],
  include_directories: include_directories('.'),
  dependencies: [function_dep, typesystem_dep],
  install: false,
)

function_alu_dep = declare_dependency(
  include_directories: include_directories('.'),
  link_with: function_alu_lib,
)
