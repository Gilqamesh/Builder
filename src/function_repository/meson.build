project('function_repository', 'cpp')

function_dep = subproject('../function').get_variable('function_dep')

function_repository_lib = library(
  'function_repository',
  ['function_repository.cpp'],
  include_directories: include_directories('.'),
  dependencies: [function_dep],
  install: false,
)

function_repository_dep = declare_dependency(
  include_directories: include_directories('.'),
  link_with: function_repository_lib,
)
