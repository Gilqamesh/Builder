inc = include_directories('.')

function_dep = dependency('function')
function_id_dep = dependency('function_id')

function_call_repo_lib = library(
  'function_call_repository',
  ['function_call_repository.cpp'],
  include_directories: inc,
  dependencies: [function_dep, function_id_dep],
  install: false,
)
function_call_repository_dep = declare_dependency(
  link_with: function_call_repo_lib,
  include_directories: inc,
  dependencies: [function_dep, function_id_dep],
)
meson.override_dependency('function_call_repository', function_call_repository_dep)
