project('typesystem_call', 'cpp', default_options: ['cpp_std=c++20'])

tests_opt = get_option('tests')
inc = include_directories('.')

call_dep = dependency('call', fallback: ['call', 'call_dep'])
typesystem_dep = dependency('typesystem', fallback: ['typesystem', 'typesystem_dep'])

typesystem_call_lib = library('typesystem_call', ['typesystem_call.cpp'],
  include_directories: inc,
  dependencies: [call_dep, typesystem_dep],
  install: false)
typesystem_call_dep = declare_dependency(link_with: typesystem_call_lib, include_directories: inc, dependencies: [call_dep, typesystem_dep])

gtest_dep = dependency('gtest', required: tests_opt.enabled(), disabler: true)
if tests_opt.enabled() or (tests_opt.auto() and gtest_dep.found())
  typesystem_call_tests = executable('typesystem_call_tests', ['typesystem_call_test.cpp'],
    dependencies: [typesystem_call_dep, gtest_dep])
  test('typesystem_call_tests', typesystem_call_tests)
endif
