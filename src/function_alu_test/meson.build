function_alu_dep = dependency('function_alu')
function_test_dep = dependency('function_test')
typesystem_test_dep = dependency('typesystem_test')
gtest_dep = dependency('gtest_support')

dependent_tests = [
  function_test_dep.get_variable('test_exe'),
  typesystem_test_dep.get_variable('test_exe'),
]

function_alu_tests = executable('function_alu_tests', ['function_alu_test.cpp'], dependencies: [function_alu_dep, gtest_dep])
test('function_alu', function_alu_tests, suite: 'function_alu', depends: dependent_tests)

function_alu_test_dep = declare_dependency(variables: {'test_exe': function_alu_tests})
meson.override_dependency('function_alu_test', function_alu_test_dep)
