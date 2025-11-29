inc = include_directories('.')

function_lib = library('function', ['function.cpp'],
  include_directories: inc,
  dependencies: [function_ir_dep, typesystem_dep],
  install: false)
function_dep = declare_dependency(link_with: function_lib, include_directories: inc, dependencies: [function_ir_dep, typesystem_dep])

gtest_dep = dependency('gtest', required: tests_opt.enabled(), disabler: true)
if tests_opt.enabled() or (tests_opt.auto() and gtest_dep.found())
  function_tests = executable('function_tests', ['function_test.cpp'], dependencies: [function_dep, gtest_dep])
  test('function_tests', function_tests)
endif
