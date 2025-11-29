project('function_compound_test', 'cpp')

function_compound_dep = dependency('function_compound')
function_test_dep = dependency('function_test')
function_id_test_dep = dependency('function_id_test')
typesystem_test_dep = dependency('typesystem_test')
gtest_dep = dependency('gtest_support')

dependent_tests = [
  function_test_dep.get_variable('test_exe'),
  function_id_test_dep.get_variable('test_exe'),
  typesystem_test_dep.get_variable('test_exe'),
]

function_compound_tests = executable(
  'function_compound_tests',
  ['function_compound_test.cpp'],
  dependencies: [function_compound_dep, gtest_dep],
)
test('function_compound', function_compound_tests, suite: 'function_compound', depends: dependent_tests)

function_compound_test_dep = declare_dependency(variables: {'test_exe': function_compound_tests})
meson.override_dependency('function_compound_test', function_compound_test_dep)
