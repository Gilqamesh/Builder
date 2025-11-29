project('function_call_repository', 'cpp', default_options: ['cpp_std=c++20'])

tests_opt = get_option('tests')
inc = include_directories('.')

function_dep = dependency('function', fallback: ['function', 'function_dep'])
function_id_dep = dependency('function_id', fallback: ['function_id', 'function_id_dep'])

function_call_repo_lib = library('function_call_repository', ['function_call_repository.cpp'],
  include_directories: inc,
  dependencies: [function_dep, function_id_dep],
  install: false)
function_call_repository_dep = declare_dependency(
  link_with: function_call_repo_lib,
  include_directories: inc,
  dependencies: [function_dep, function_id_dep],
)

gtest_dep = dependency('gtest', required: tests_opt.enabled(), disabler: true)
if tests_opt.enabled() or (tests_opt.auto() and gtest_dep.found())
  function_call_repo_tests = executable('function_call_repository_tests', ['function_call_repository_test.cpp'],
    dependencies: [function_call_repository_dep, gtest_dep])
  test('function_call_repository_tests', function_call_repo_tests)
endif
