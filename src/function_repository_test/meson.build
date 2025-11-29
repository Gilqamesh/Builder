function_repository_dep = dependency('function_repository')
function_test_dep = dependency('function_test')
gtest_dep = dependency('gtest_support')

function_repository_tests = executable(
  'function_repository_tests',
  ['function_repository_test.cpp'],
  dependencies: [function_repository_dep, gtest_dep],
)
test(
  'function_repository',
  function_repository_tests,
  suite: 'function_repository',
  depends: [function_test_dep.get_variable('test_exe')],
)

function_repository_test_dep = declare_dependency(variables: {'test_exe': function_repository_tests})
meson.override_dependency('function_repository_test', function_repository_test_dep)
