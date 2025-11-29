project('function_ir_file_repository', 'cpp', default_options: ['cpp_std=c++20'])

tests_opt = get_option('tests')
inc = include_directories('.')

function_ir_dep = dependency('function_ir', fallback: ['function_ir', 'function_ir_dep'])
function_ir_binary_dep = dependency('function_ir_binary', fallback: ['function_ir_binary', 'function_ir_binary_dep'])
typesystem_dep = dependency('typesystem', fallback: ['typesystem', 'typesystem_dep'])

function_ir_file_repo_lib = library('function_ir_file_repository', ['function_ir_file_repository.cpp'],
  include_directories: inc,
  dependencies: [function_ir_dep, function_ir_binary_dep, typesystem_dep],
  install: false)
function_ir_file_repository_dep = declare_dependency(
  link_with: function_ir_file_repo_lib,
  include_directories: inc,
  dependencies: [function_ir_dep, function_ir_binary_dep, typesystem_dep],
)

gtest_dep = dependency('gtest', required: tests_opt.enabled(), disabler: true)
if tests_opt.enabled() or (tests_opt.auto() and gtest_dep.found())
  function_ir_file_repo_tests = executable('function_ir_file_repository_tests', ['function_ir_file_repository_test.cpp'],
    dependencies: [function_ir_file_repository_dep, gtest_dep])
  test('function_ir_file_repository_tests', function_ir_file_repo_tests)
endif
