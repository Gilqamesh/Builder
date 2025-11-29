project('function_call_repository_test', 'cpp')

function_call_repository_dep = subproject('../function_call_repository').get_variable('function_call_repository_dep')
function_test = subproject('../function_test')
function_id_test = subproject('../function_id_test')
gtest_dep = subproject('../gtest').get_variable('gtest_dep')

dependent_tests = [
  function_test.get_variable('test_exe'),
  function_id_test.get_variable('test_exe'),
]

function_call_repository_tests = executable(
  'function_call_repository_tests',
  ['function_call_repository_test.cpp'],
  dependencies: [function_call_repository_dep, gtest_dep],
)

test(
  'function_call_repository',
  function_call_repository_tests,
  suite: 'function_call_repository',
  depends: dependent_tests,
)

test_exe = function_call_repository_tests
