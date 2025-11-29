project('function_primitive_cpp_test', 'cpp')

function_primitive_cpp_dep = subproject('../function_primitive_cpp').get_variable('function_primitive_cpp_dep')
function_test = subproject('../function_test')
gtest_dep = subproject('../gtest').get_variable('gtest_dep')

function_primitive_cpp_tests = executable(
  'function_primitive_cpp_tests',
  ['function_primitive_cpp_test.cpp'],
  dependencies: [function_primitive_cpp_dep, gtest_dep],
)

test(
  'function_primitive_cpp',
  function_primitive_cpp_tests,
  suite: 'function_primitive_cpp',
  depends: [function_test.get_variable('test_exe')],
)

test_exe = function_primitive_cpp_tests
