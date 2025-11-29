project('typesystem_call_test', 'cpp')

typesystem_call_dep = subproject('../typesystem_call').get_variable('typesystem_call_dep')
call_test = subproject('../call_test')
typesystem_test = subproject('../typesystem_test')
gtest_dep = subproject('../gtest').get_variable('gtest_dep')

dependent_tests = [
  call_test.get_variable('test_exe'),
  typesystem_test.get_variable('test_exe'),
]

typesystem_call_tests = executable(
  'typesystem_call_tests',
  ['typesystem_call_test.cpp'],
  dependencies: [typesystem_call_dep, gtest_dep],
)

test('typesystem_call', typesystem_call_tests, suite: 'typesystem_call', depends: dependent_tests)

test_exe = typesystem_call_tests
