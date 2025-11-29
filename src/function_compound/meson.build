project('function_compound', 'cpp', default_options: ['cpp_std=c++20'])

tests_opt = get_option('tests')
inc = include_directories('.')

function_dep = dependency('function', fallback: ['function', 'function_dep'])
function_id_dep = dependency('function_id', fallback: ['function_id', 'function_id_dep'])
typesystem_dep = dependency('typesystem', fallback: ['typesystem', 'typesystem_dep'])

function_compound_lib = library('function_compound', ['function_compound.cpp'],
  include_directories: inc,
  dependencies: [function_dep, function_id_dep, typesystem_dep],
  install: false)
function_compound_dep = declare_dependency(
  link_with: function_compound_lib,
  include_directories: inc,
  dependencies: [function_dep, function_id_dep, typesystem_dep],
)

gtest_dep = dependency('gtest', required: tests_opt.enabled(), disabler: true)
if tests_opt.enabled() or (tests_opt.auto() and gtest_dep.found())
  function_compound_tests = executable('function_compound_tests', ['function_compound_test.cpp'],
    dependencies: [function_compound_dep, gtest_dep])
  test('function_compound_tests', function_compound_tests)
endif
