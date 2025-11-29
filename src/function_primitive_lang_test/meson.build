project('function_primitive_lang_test', 'cpp')

function_primitive_lang_dep = dependency('function_primitive_lang')
function_test_dep = dependency('function_test')
gtest_dep = dependency('gtest_support')

function_primitive_lang_tests = executable(
  'function_primitive_lang_tests',
  ['function_primitive_lang_test.cpp'],
  dependencies: [function_primitive_lang_dep, gtest_dep],
)
test(
  'function_primitive_lang',
  function_primitive_lang_tests,
  suite: 'function_primitive_lang',
  depends: [function_test_dep.get_variable('test_exe')],
)

function_primitive_lang_test_dep = declare_dependency(variables: {'test_exe': function_primitive_lang_tests})
meson.override_dependency('function_primitive_lang_test', function_primitive_lang_test_dep)
