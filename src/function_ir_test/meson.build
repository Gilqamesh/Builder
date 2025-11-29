project('function_ir_test', 'cpp')

function_ir_dep = subproject('../function_ir').get_variable('function_ir_dep')
function_id_test = subproject('../function_id_test')
gtest_dep = subproject('../gtest').get_variable('gtest_dep')

function_ir_tests = executable(
  'function_ir_tests',
  ['function_ir_test.cpp'],
  dependencies: [function_ir_dep, gtest_dep],
)

test(
  'function_ir',
  function_ir_tests,
  suite: 'function_ir',
  depends: [function_id_test.get_variable('test_exe')],
)

test_exe = function_ir_tests
