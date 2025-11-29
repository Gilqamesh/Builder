project('function_call_repository', 'cpp')

function_dep = subproject('../function').get_variable('function_dep')
function_id_dep = subproject('../function_id').get_variable('function_id_dep')

function_call_repository_lib = library(
  'function_call_repository',
  ['function_call_repository.cpp'],
  include_directories: include_directories('.'),
  dependencies: [function_dep, function_id_dep],
  install: false,
)

function_call_repository_dep = declare_dependency(
  include_directories: include_directories('.'),
  link_with: function_call_repository_lib,
)
