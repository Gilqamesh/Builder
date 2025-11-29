project('function_alu_test', 'cpp')

function_alu_dep = subproject('../function_alu').get_variable('function_alu_dep')
function_test = subproject('../function_test')
typesystem_test = subproject('../typesystem_test')
gtest_dep = subproject('../gtest').get_variable('gtest_dep')

dependent_tests = [
  function_test.get_variable('test_exe'),
  typesystem_test.get_variable('test_exe'),
]

function_alu_tests = executable(
  'function_alu_tests',
  ['function_alu_test.cpp'],
  dependencies: [function_alu_dep, gtest_dep],
)

test('function_alu', function_alu_tests, suite: 'function_alu', depends: dependent_tests)

test_exe = function_alu_tests
