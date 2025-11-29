inc = include_directories('.')

function_dep = dependency('function')
typesystem_dep = dependency('typesystem')

function_alu_lib = library(
  'function_alu',
  ['function_alu.cpp'],
  include_directories: inc,
  dependencies: [function_dep, typesystem_dep],
  install: false,
)
function_alu_dep = declare_dependency(
  link_with: function_alu_lib,
  include_directories: inc,
  dependencies: [function_dep, typesystem_dep],
)
meson.override_dependency('function_alu', function_alu_dep)
