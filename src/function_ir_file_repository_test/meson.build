project('function_ir_file_repository_test', 'cpp')

function_ir_file_repository_dep = subproject('../function_ir_file_repository').get_variable('function_ir_file_repository_dep')
function_ir_test = subproject('../function_ir_test')
function_ir_binary_test = subproject('../function_ir_binary_test')
typesystem_test = subproject('../typesystem_test')
gtest_dep = subproject('../gtest').get_variable('gtest_dep')

dependent_tests = [
  function_ir_test.get_variable('test_exe'),
  function_ir_binary_test.get_variable('test_exe'),
  typesystem_test.get_variable('test_exe'),
]

function_ir_file_repository_tests = executable(
  'function_ir_file_repository_tests',
  ['function_ir_file_repository_test.cpp'],
  dependencies: [function_ir_file_repository_dep, gtest_dep],
)

test(
  'function_ir_file_repository',
  function_ir_file_repository_tests,
  suite: 'function_ir_file_repository',
  depends: dependent_tests,
)

test_exe = function_ir_file_repository_tests
