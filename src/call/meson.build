project('call', 'cpp')

call_lib = library(
  'call',
  ['call.cpp'],
  include_directories: include_directories('.'),
  install: false,
)

call_dep = declare_dependency(
  include_directories: include_directories('.'),
  link_with: call_lib,
)
