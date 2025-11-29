project('function_ir_assembly_test', 'cpp')

function_ir_assembly_dep = dependency('function_ir_assembly')
function_ir_test_dep = dependency('function_ir_test')
gtest_dep = dependency('gtest_support')

function_ir_assembly_tests = executable(
  'function_ir_assembly_tests',
  ['function_ir_assembly_test.cpp'],
  dependencies: [function_ir_assembly_dep, gtest_dep],
)
test(
  'function_ir_assembly',
  function_ir_assembly_tests,
  suite: 'function_ir_assembly',
  depends: [function_ir_test_dep.get_variable('test_exe')],
)

function_ir_assembly_test_dep = declare_dependency(variables: {'test_exe': function_ir_assembly_tests})
meson.override_dependency('function_ir_assembly_test', function_ir_assembly_test_dep)
