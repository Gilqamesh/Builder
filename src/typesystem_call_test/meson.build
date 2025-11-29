project('typesystem_call_test', 'cpp')

typesystem_call_dep = dependency('typesystem_call')
call_test_dep = dependency('call_test')
typesystem_test_dep = dependency('typesystem_test')
gtest_dep = dependency('gtest_support')

dependent_tests = [
  call_test_dep.get_variable('test_exe'),
  typesystem_test_dep.get_variable('test_exe'),
]

typesystem_call_tests = executable(
  'typesystem_call_tests',
  ['typesystem_call_test.cpp'],
  dependencies: [typesystem_call_dep, gtest_dep],
)
test('typesystem_call', typesystem_call_tests, suite: 'typesystem_call', depends: dependent_tests)

typesystem_call_test_dep = declare_dependency(variables: {'test_exe': typesystem_call_tests})
meson.override_dependency('typesystem_call_test', typesystem_call_test_dep)
