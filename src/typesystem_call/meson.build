project('typesystem_call', 'cpp')

inc = include_directories('.')

call_dep = dependency('call')
typesystem_dep = dependency('typesystem')

typesystem_call_lib = library(
  'typesystem_call',
  ['typesystem_call.cpp'],
  include_directories: inc,
  dependencies: [call_dep, typesystem_dep],
  install: false,
)
typesystem_call_dep = declare_dependency(
  link_with: typesystem_call_lib,
  include_directories: inc,
  dependencies: [call_dep, typesystem_dep],
)
meson.override_dependency('typesystem_call', typesystem_call_dep)
