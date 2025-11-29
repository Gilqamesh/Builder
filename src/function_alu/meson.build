inc = include_directories('.')

function_alu_lib = library('function_alu', ['function_alu.cpp'],
  include_directories: inc,
  dependencies: [function_dep, typesystem_dep],
  install: false)
function_alu_dep = declare_dependency(link_with: function_alu_lib, include_directories: inc, dependencies: [function_dep, typesystem_dep])

gtest_dep = dependency('gtest', required: tests_opt.enabled(), disabler: true)
if tests_opt.enabled() or (tests_opt.auto() and gtest_dep.found())
  function_alu_tests = executable('function_alu_tests', ['function_alu_test.cpp'], dependencies: [function_alu_dep, gtest_dep])
  test('function_alu_tests', function_alu_tests)
endif
