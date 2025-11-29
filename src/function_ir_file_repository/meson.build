project('function_ir_file_repository', 'cpp')

inc = include_directories('.')

function_ir_dep = dependency('function_ir')
function_ir_binary_dep = dependency('function_ir_binary')
typesystem_dep = dependency('typesystem')

function_ir_file_repo_lib = library(
  'function_ir_file_repository',
  ['function_ir_file_repository.cpp'],
  include_directories: inc,
  dependencies: [function_ir_dep, function_ir_binary_dep, typesystem_dep],
  install: false,
)
function_ir_file_repository_dep = declare_dependency(
  link_with: function_ir_file_repo_lib,
  include_directories: inc,
  dependencies: [function_ir_dep, function_ir_binary_dep, typesystem_dep],
)
meson.override_dependency('function_ir_file_repository', function_ir_file_repository_dep)
