project('function_repository_test', 'cpp')

function_repository_dep = subproject('../function_repository').get_variable('function_repository_dep')
function_test = subproject('../function_test')
gtest_dep = subproject('../gtest').get_variable('gtest_dep')

function_repository_tests = executable(
  'function_repository_tests',
  ['function_repository_test.cpp'],
  dependencies: [function_repository_dep, gtest_dep],
)

test(
  'function_repository',
  function_repository_tests,
  suite: 'function_repository',
  depends: [function_test.get_variable('test_exe')],
)

test_exe = function_repository_tests
