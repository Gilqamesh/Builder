project('function_ir_test', 'cpp')

function_ir_dep = dependency('function_ir')
function_id_test_dep = dependency('function_id_test')
gtest_dep = dependency('gtest_support')

function_ir_tests = executable('function_ir_tests', ['function_ir_test.cpp'], dependencies: [function_ir_dep, gtest_dep])
test(
  'function_ir',
  function_ir_tests,
  suite: 'function_ir',
  depends: [function_id_test_dep.get_variable('test_exe')],
)

function_ir_test_dep = declare_dependency(variables: {'test_exe': function_ir_tests})
meson.override_dependency('function_ir_test', function_ir_test_dep)
