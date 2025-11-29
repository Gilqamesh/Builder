project('function_repository', 'cpp', default_options: ['cpp_std=c++20'])

tests_opt = get_option('tests')
inc = include_directories('.')

function_dep = dependency('function', fallback: ['function', 'function_dep'])

function_repository_lib = library('function_repository', ['function_repository.cpp'],
  include_directories: inc,
  dependencies: [function_dep],
  install: false)
function_repository_dep = declare_dependency(link_with: function_repository_lib, include_directories: inc, dependencies: [function_dep])

gtest_dep = dependency('gtest', required: tests_opt.enabled(), disabler: true)
if tests_opt.enabled() or (tests_opt.auto() and gtest_dep.found())
  function_repository_tests = executable('function_repository_tests', ['function_repository_test.cpp'],
    dependencies: [function_repository_dep, gtest_dep])
  test('function_repository_tests', function_repository_tests)
endif
