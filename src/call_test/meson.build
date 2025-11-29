project('call_test', 'cpp')

call_dep = subproject('../call').get_variable('call_dep')
gtest_dep = subproject('../gtest').get_variable('gtest_dep')

call_tests = executable(
  'call_tests',
  ['call_test.cpp'],
  dependencies: [call_dep, gtest_dep],
)

test('call', call_tests, suite: 'call')

test_exe = call_tests
