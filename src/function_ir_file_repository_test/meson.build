project('function_ir_file_repository_test', 'cpp')

function_ir_file_repository_dep = dependency('function_ir_file_repository')
function_ir_test_dep = dependency('function_ir_test')
function_ir_binary_test_dep = dependency('function_ir_binary_test')
typesystem_test_dep = dependency('typesystem_test')
gtest_dep = dependency('gtest_support')

dependent_tests = [
  function_ir_test_dep.get_variable('test_exe'),
  function_ir_binary_test_dep.get_variable('test_exe'),
  typesystem_test_dep.get_variable('test_exe'),
]

function_ir_file_repo_tests = executable(
  'function_ir_file_repository_tests',
  ['function_ir_file_repository_test.cpp'],
  dependencies: [function_ir_file_repository_dep, gtest_dep],
)
test(
  'function_ir_file_repository',
  function_ir_file_repo_tests,
  suite: 'function_ir_file_repository',
  depends: dependent_tests,
)

function_ir_file_repository_test_dep = declare_dependency(variables: {'test_exe': function_ir_file_repo_tests})
meson.override_dependency('function_ir_file_repository_test', function_ir_file_repository_test_dep)
