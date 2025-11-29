project('call_test', 'cpp')

call_dep = dependency('call')
gtest_dep = dependency('gtest_support')

call_tests = executable('call_tests', ['call_test.cpp'], dependencies: [call_dep, gtest_dep])
test('call', call_tests, suite: 'call')

call_test_dep = declare_dependency(variables: {'test_exe': call_tests})
meson.override_dependency('call_test', call_test_dep)
