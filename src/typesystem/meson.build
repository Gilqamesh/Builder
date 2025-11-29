project('typesystem', 'cpp', default_options: ['cpp_std=c++20'])

tests_opt = get_option('tests')
inc = include_directories('.')

typesystem_lib = library('typesystem', ['typesystem.cpp'], include_directories: inc, install: false)
typesystem_dep = declare_dependency(link_with: typesystem_lib, include_directories: inc)

gtest_dep = dependency('gtest', required: tests_opt.enabled(), disabler: true)
if tests_opt.enabled() or (tests_opt.auto() and gtest_dep.found())
  typesystem_tests = executable('typesystem_tests', ['typesystem_test.cpp'], dependencies: [typesystem_dep, gtest_dep])
  test('typesystem_tests', typesystem_tests)
endif
