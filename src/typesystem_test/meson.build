typesystem_dep = dependency('typesystem')
gtest_dep = dependency('gtest_support')

typesystem_tests = executable('typesystem_tests', ['typesystem_test.cpp'], dependencies: [typesystem_dep, gtest_dep])
test('typesystem', typesystem_tests, suite: 'typesystem')

typesystem_test_dep = declare_dependency(variables: {'test_exe': typesystem_tests})
meson.override_dependency('typesystem_test', typesystem_test_dep)
