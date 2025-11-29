project('function_ir_assembly_test', 'cpp')

function_ir_assembly_dep = subproject('../function_ir_assembly').get_variable('function_ir_assembly_dep')
function_ir_test = subproject('../function_ir_test')
gtest_dep = subproject('../gtest').get_variable('gtest_dep')

function_ir_assembly_tests = executable(
  'function_ir_assembly_tests',
  ['function_ir_assembly_test.cpp'],
  dependencies: [function_ir_assembly_dep, gtest_dep],
)

test(
  'function_ir_assembly',
  function_ir_assembly_tests,
  suite: 'function_ir_assembly',
  depends: [function_ir_test.get_variable('test_exe')],
)

test_exe = function_ir_assembly_tests
