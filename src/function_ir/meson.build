inc = include_directories('.')

function_ir_lib = library('function_ir', ['function_ir.cpp'],
  include_directories: inc,
  dependencies: [function_id_dep],
  install: false)
function_ir_dep = declare_dependency(link_with: function_ir_lib, include_directories: inc, dependencies: [function_id_dep])

gtest_dep = dependency('gtest', required: tests_opt.enabled(), disabler: true)
if tests_opt.enabled() or (tests_opt.auto() and gtest_dep.found())
  function_ir_tests = executable('function_ir_tests', ['function_ir_test.cpp'], dependencies: [function_ir_dep, gtest_dep])
  test('function_ir_tests', function_ir_tests)
endif
