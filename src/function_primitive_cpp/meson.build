inc = include_directories('.')

function_primitive_cpp_lib = library('function_primitive_cpp', ['function_primitive_cpp.cpp'],
  include_directories: inc,
  dependencies: [function_dep],
  install: false)
function_primitive_cpp_dep = declare_dependency(link_with: function_primitive_cpp_lib, include_directories: inc, dependencies: [function_dep])

gtest_dep = dependency('gtest', required: tests_opt.enabled(), disabler: true)
if tests_opt.enabled() or (tests_opt.auto() and gtest_dep.found())
  function_primitive_cpp_tests = executable('function_primitive_cpp_tests', ['function_primitive_cpp_test.cpp'],
    dependencies: [function_primitive_cpp_dep, gtest_dep])
  test('function_primitive_cpp_tests', function_primitive_cpp_tests)
endif
