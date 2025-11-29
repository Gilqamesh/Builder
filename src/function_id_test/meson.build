project('function_id_test', 'cpp')

function_id_dep = dependency('function_id')
gtest_dep = dependency('gtest_support')

function_id_tests = executable('function_id_tests', ['function_id_test.cpp'], dependencies: [function_id_dep, gtest_dep])
test('function_id', function_id_tests, suite: 'function_id')

function_id_test_dep = declare_dependency(variables: {'test_exe': function_id_tests})
meson.override_dependency('function_id_test', function_id_test_dep)
