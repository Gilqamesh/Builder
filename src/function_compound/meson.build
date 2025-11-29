inc = include_directories('.')

function_dep = dependency('function')
function_id_dep = dependency('function_id')
typesystem_dep = dependency('typesystem')

function_compound_lib = library(
  'function_compound',
  ['function_compound.cpp'],
  include_directories: inc,
  dependencies: [function_dep, function_id_dep, typesystem_dep],
  install: false,
)
function_compound_dep = declare_dependency(
  link_with: function_compound_lib,
  include_directories: inc,
  dependencies: [function_dep, function_id_dep, typesystem_dep],
)
meson.override_dependency('function_compound', function_compound_dep)
