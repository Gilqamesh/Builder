project('call', 'cpp', default_options: ['cpp_std=c++20'])

tests_opt = get_option('tests')
inc = include_directories('.')

call_lib = library('call', ['call.cpp'], include_directories: inc, install: false)
call_dep = declare_dependency(link_with: call_lib, include_directories: inc)

gtest_main = dependency('gtest_main')
gtest = dependency('gtest')
gtest_dep = [gtest, gtest_main]
if tests_opt.enabled() or (tests_opt.auto() and gtest_dep.found())
  call_tests = executable('call_tests', ['call_test.cpp'], dependencies: [call_dep, gtest_dep])
  test('call_tests', call_tests)
endif
