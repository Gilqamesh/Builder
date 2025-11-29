project('function_primitive_lang', 'cpp')

inc = include_directories('.')

function_dep = dependency('function')

function_primitive_lang_lib = library(
  'function_primitive_lang',
  ['function_primitive_lang.cpp'],
  include_directories: inc,
  dependencies: [function_dep],
  install: false,
)
function_primitive_lang_dep = declare_dependency(
  link_with: function_primitive_lang_lib,
  include_directories: inc,
  dependencies: [function_dep],
)
meson.override_dependency('function_primitive_lang', function_primitive_lang_dep)
