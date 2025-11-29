project('function_ir_binary_test', 'cpp')

function_ir_binary_dep = dependency('function_ir_binary')
function_ir_test_dep = dependency('function_ir_test')
gtest_dep = dependency('gtest_support')

function_ir_binary_tests = executable(
  'function_ir_binary_tests',
  ['function_ir_binary_test.cpp'],
  dependencies: [function_ir_binary_dep, gtest_dep],
)
test(
  'function_ir_binary',
  function_ir_binary_tests,
  suite: 'function_ir_binary',
  depends: [function_ir_test_dep.get_variable('test_exe')],
)

function_ir_binary_test_dep = declare_dependency(variables: {'test_exe': function_ir_binary_tests})
meson.override_dependency('function_ir_binary_test', function_ir_binary_test_dep)
