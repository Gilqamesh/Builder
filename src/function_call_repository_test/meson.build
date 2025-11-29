function_call_repository_dep = dependency('function_call_repository')
function_test_dep = dependency('function_test')
gtest_dep = dependency('gtest_support')
function_id_test_dep = dependency('function_id_test')

function_call_repo_tests = executable(
  'function_call_repository_tests',
  ['function_call_repository_test.cpp'],
  dependencies: [function_call_repository_dep, gtest_dep],
)
test(
  'function_call_repository',
  function_call_repo_tests,
  suite: 'function_call_repository',
  depends: [
    function_test_dep.get_variable('test_exe'),
    function_id_test_dep.get_variable('test_exe'),
  ],
)

function_call_repository_test_dep = declare_dependency(variables: {'test_exe': function_call_repo_tests})
meson.override_dependency('function_call_repository_test', function_call_repository_test_dep)
