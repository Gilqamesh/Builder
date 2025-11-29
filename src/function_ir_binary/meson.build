project('function_ir_binary', 'cpp')

inc = include_directories('.')

function_ir_dep = dependency('function_ir')

function_ir_binary_lib = library(
  'function_ir_binary',
  ['function_ir_binary.cpp'],
  include_directories: inc,
  dependencies: [function_ir_dep],
  install: false,
)
function_ir_binary_dep = declare_dependency(
  link_with: function_ir_binary_lib,
  include_directories: inc,
  dependencies: [function_ir_dep],
)
meson.override_dependency('function_ir_binary', function_ir_binary_dep)
