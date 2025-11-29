inc = include_directories('.')

call_lib = library('call', ['call.cpp'], include_directories: inc, install: false)
call_dep = declare_dependency(link_with: call_lib, include_directories: inc)
meson.override_dependency('call', call_dep)
