inc = include_directories('.')

function_primitive_lang_lib = library('function_primitive_lang', ['function_primitive_lang.cpp'],
  include_directories: inc,
  dependencies: [function_dep],
  install: false)
function_primitive_lang_dep = declare_dependency(link_with: function_primitive_lang_lib, include_directories: inc, dependencies: [function_dep])

gtest_dep = dependency('gtest', required: tests_opt.enabled(), disabler: true)
if tests_opt.enabled() or (tests_opt.auto() and gtest_dep.found())
  function_primitive_lang_tests = executable('function_primitive_lang_tests', ['function_primitive_lang_test.cpp'],
    dependencies: [function_primitive_lang_dep, gtest_dep])
  test('function_primitive_lang_tests', function_primitive_lang_tests)
endif
