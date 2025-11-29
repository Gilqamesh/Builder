inc = include_directories('.')

function_ir_dep = dependency('function_ir')
typesystem_dep = dependency('typesystem')

function_lib = library(
  'function',
  ['function.cpp'],
  include_directories: inc,
  dependencies: [function_ir_dep, typesystem_dep],
  install: false,
)
function_dep = declare_dependency(
  link_with: function_lib,
  include_directories: inc,
  dependencies: [function_ir_dep, typesystem_dep],
)
meson.override_dependency('function', function_dep)
