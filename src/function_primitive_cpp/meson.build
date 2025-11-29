project('function_primitive_cpp', 'cpp')

function_dep = subproject('../function').get_variable('function_dep')

function_primitive_cpp_lib = library(
  'function_primitive_cpp',
  ['function_primitive_cpp.cpp'],
  include_directories: include_directories('.'),
  dependencies: [function_dep],
  install: false,
)

function_primitive_cpp_dep = declare_dependency(
  include_directories: include_directories('.'),
  link_with: function_primitive_cpp_lib,
)
