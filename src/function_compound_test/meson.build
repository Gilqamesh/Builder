project('function_compound_test', 'cpp')

function_compound_dep = subproject('../function_compound').get_variable('function_compound_dep')
function_test = subproject('../function_test')
function_id_test = subproject('../function_id_test')
typesystem_test = subproject('../typesystem_test')
gtest_dep = subproject('../gtest').get_variable('gtest_dep')

dependent_tests = [
  function_test.get_variable('test_exe'),
  function_id_test.get_variable('test_exe'),
  typesystem_test.get_variable('test_exe'),
]

function_compound_tests = executable(
  'function_compound_tests',
  ['function_compound_test.cpp'],
  dependencies: [function_compound_dep, gtest_dep],
)

test('function_compound', function_compound_tests, suite: 'function_compound', depends: dependent_tests)

test_exe = function_compound_tests
