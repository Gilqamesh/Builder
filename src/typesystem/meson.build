project('typesystem', 'cpp')

typesystem_lib = library(
  'typesystem',
  ['typesystem.cpp'],
  include_directories: include_directories('.'),
)

typesystem_dep = declare_dependency(
  include_directories: include_directories('.'),
  link_with: typesystem_lib,
)
