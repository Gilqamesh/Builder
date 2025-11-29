inc = include_directories('.')

function_dep = dependency('function')

function_repository_lib = library(
  'function_repository',
  ['function_repository.cpp'],
  include_directories: inc,
  dependencies: [function_dep],
  install: false,
)
function_repository_dep = declare_dependency(
  link_with: function_repository_lib,
  include_directories: inc,
  dependencies: [function_dep],
)
meson.override_dependency('function_repository', function_repository_dep)
