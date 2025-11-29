project('function_primitive_lang_test', 'cpp')

function_primitive_lang_dep = subproject('../function_primitive_lang').get_variable('function_primitive_lang_dep')
function_test = subproject('../function_test')
gtest_dep = subproject('../gtest').get_variable('gtest_dep')

function_primitive_lang_tests = executable(
  'function_primitive_lang_tests',
  ['function_primitive_lang_test.cpp'],
  dependencies: [function_primitive_lang_dep, gtest_dep],
)

test(
  'function_primitive_lang',
  function_primitive_lang_tests,
  suite: 'function_primitive_lang',
  depends: [function_test.get_variable('test_exe')],
)

test_exe = function_primitive_lang_tests
