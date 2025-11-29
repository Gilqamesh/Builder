inc = include_directories('.')

function_id_lib = library('function_id', ['function_id.cpp'], include_directories: inc, install: false)
function_id_dep = declare_dependency(link_with: function_id_lib, include_directories: inc)
meson.override_dependency('function_id', function_id_dep)
