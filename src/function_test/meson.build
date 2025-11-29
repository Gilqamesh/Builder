project('function_test', 'cpp')

function_dep = subproject('../function').get_variable('function_dep')
gtest_dep = subproject('../gtest').get_variable('gtest_dep')

function_tests = executable(
  'function_test',
  ['function_test.cpp'],
  dependencies: [function_dep, gtest_dep],
)

test('function_test', function_tests)

test_exe = function_tests
