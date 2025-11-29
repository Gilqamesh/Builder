project('typesystem_test', 'cpp')

typesystem_dep = subproject('../typesystem').get_variable('typesystem_dep')
gtest_dep = subproject('../gtest').get_variable('gtest_dep')

typesystem_tests = executable(
  'typesystem_tests',
  ['typesystem_test.cpp'],
  dependencies: [typesystem_dep, gtest_dep],
)

test('typesystem', typesystem_tests, suite: 'typesystem')

test_exe = typesystem_tests
