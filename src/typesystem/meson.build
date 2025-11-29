inc = include_directories('.')

typesystem_lib = library('typesystem', ['typesystem.cpp'], include_directories: inc, install: false)
typesystem_dep = declare_dependency(link_with: typesystem_lib, include_directories: inc)
meson.override_dependency('typesystem', typesystem_dep)
