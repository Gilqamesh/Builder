project('function_primitive_cpp_test', 'cpp')

function_primitive_cpp_dep = dependency('function_primitive_cpp')
function_test_dep = dependency('function_test')
gtest_dep = dependency('gtest_support')

function_primitive_cpp_tests = executable(
  'function_primitive_cpp_tests',
  ['function_primitive_cpp_test.cpp'],
  dependencies: [function_primitive_cpp_dep, gtest_dep],
)
test(
  'function_primitive_cpp',
  function_primitive_cpp_tests,
  suite: 'function_primitive_cpp',
  depends: [function_test_dep.get_variable('test_exe')],
)

function_primitive_cpp_test_dep = declare_dependency(variables: {'test_exe': function_primitive_cpp_tests})
meson.override_dependency('function_primitive_cpp_test', function_primitive_cpp_test_dep)
