project('function_id', 'cpp')

function_id_lib = library(
  'function_id',
  ['function_id.cpp'],
  include_directories: include_directories('.'),
)

function_id_dep = declare_dependency(
  include_directories: include_directories('.'),
  link_with: function_id_lib,
)
