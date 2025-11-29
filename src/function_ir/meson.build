inc = include_directories('.')

function_id_dep = dependency('function_id')

function_ir_lib = library(
  'function_ir',
  ['function_ir.cpp'],
  include_directories: inc,
  dependencies: [function_id_dep],
  install: false,
)
function_ir_dep = declare_dependency(
  link_with: function_ir_lib,
  include_directories: inc,
  dependencies: [function_id_dep],
)
meson.override_dependency('function_ir', function_ir_dep)
