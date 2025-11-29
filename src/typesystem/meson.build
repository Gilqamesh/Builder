project('typesystem', 'cpp')

typesystem_lib = library(
  'typesystem',
  ['typesystem.cpp'],
)

typesystem_dep = declare_dependency(
  include_directories: include_directories('.'),
  link_with: typesystem_lib,
)

meson.override_dependency('typesystem', typesystem_dep)
