project('function_ir_binary_test', 'cpp')

function_ir_binary_dep = subproject('../function_ir_binary').get_variable('function_ir_binary_dep')
function_ir_test = subproject('../function_ir_test')
gtest_dep = subproject('../gtest').get_variable('gtest_dep')

function_ir_binary_tests = executable(
  'function_ir_binary_tests',
  ['function_ir_binary_test.cpp'],
  dependencies: [function_ir_binary_dep, gtest_dep],
)

test(
  'function_ir_binary',
  function_ir_binary_tests,
  suite: 'function_ir_binary',
  depends: [function_ir_test.get_variable('test_exe')],
)

test_exe = function_ir_binary_tests
