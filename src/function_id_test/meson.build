project('function_id_test', 'cpp')

function_id_dep = subproject('../function_id').get_variable('function_id_dep')
gtest_dep = subproject('../gtest').get_variable('gtest_dep')

function_id_tests = executable(
  'function_id_tests',
  ['function_id_test.cpp'],
  dependencies: [function_id_dep, gtest_dep],
)

test('function_id', function_id_tests, suite: 'function_id')

test_exe = function_id_tests
