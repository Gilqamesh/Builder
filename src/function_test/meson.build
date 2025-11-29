function_dep = dependency('function')
function_ir_test_dep = dependency('function_ir_test')
typesystem_test_dep = dependency('typesystem_test')
gtest_dep = dependency('gtest_support')

dependent_tests = [
  function_ir_test_dep.get_variable('test_exe'),
  typesystem_test_dep.get_variable('test_exe'),
]

function_tests = executable('function_tests', ['function_test.cpp'], dependencies: [function_dep, gtest_dep])
test('function', function_tests, suite: 'function', depends: dependent_tests)

function_test_dep = declare_dependency(variables: {'test_exe': function_tests})
meson.override_dependency('function_test', function_test_dep)
