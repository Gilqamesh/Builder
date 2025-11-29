project('function_compound', 'cpp')

function_dep = subproject('../function').get_variable('function_dep')
function_id_dep = subproject('../function_id').get_variable('function_id_dep')
typesystem_dep = subproject('../typesystem').get_variable('typesystem_dep')

function_compound_lib = library(
  'function_compound',
  ['function_compound.cpp'],
  include_directories: include_directories('.'),
  dependencies: [function_dep, function_id_dep, typesystem_dep],
  install: false,
)

function_compound_dep = declare_dependency(
  include_directories: include_directories('.'),
  link_with: function_compound_lib,
)
