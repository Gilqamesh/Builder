project('function_ir_binary', 'cpp', default_options: ['cpp_std=c++20'])

tests_opt = get_option('tests')
inc = include_directories('.')

function_ir_dep = dependency('function_ir', fallback: ['function_ir', 'function_ir_dep'])

function_ir_binary_lib = library('function_ir_binary', ['function_ir_binary.cpp'],
  include_directories: inc,
  dependencies: [function_ir_dep],
  install: false)
function_ir_binary_dep = declare_dependency(link_with: function_ir_binary_lib, include_directories: inc, dependencies: [function_ir_dep])

gtest_dep = dependency('gtest', required: tests_opt.enabled(), disabler: true)
if tests_opt.enabled() or (tests_opt.auto() and gtest_dep.found())
  function_ir_binary_tests = executable('function_ir_binary_tests', ['function_ir_binary_test.cpp'],
    dependencies: [function_ir_binary_dep, gtest_dep])
  test('function_ir_binary_tests', function_ir_binary_tests)
endif
