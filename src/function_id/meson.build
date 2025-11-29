inc = include_directories('.')

function_id_lib = library('function_id', ['function_id.cpp'], include_directories: inc, install: false)
function_id_dep = declare_dependency(link_with: function_id_lib, include_directories: inc)

gtest_dep = dependency('gtest', required: tests_opt.enabled(), disabler: true)
if tests_opt.enabled() or (tests_opt.auto() and gtest_dep.found())
  function_id_tests = executable('function_id_tests', ['function_id_test.cpp'], dependencies: [function_id_dep, gtest_dep])
  test('function_id_tests', function_id_tests)
endif
