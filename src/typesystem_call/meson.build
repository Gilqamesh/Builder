project('typesystem_call', 'cpp')

call_dep = subproject('../call').get_variable('call_dep')
typesystem_dep = subproject('../typesystem').get_variable('typesystem_dep')

typesystem_call_lib = library(
  'typesystem_call',
  ['typesystem_call.cpp'],
  include_directories: include_directories('.'),
  dependencies: [call_dep, typesystem_dep],
  install: false,
)

typesystem_call_dep = declare_dependency(
  include_directories: include_directories('.'),
  link_with: typesystem_call_lib,
)
