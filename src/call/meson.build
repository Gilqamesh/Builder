inc = include_directories('.')

call_lib = library('call', ['call.cpp'], include_directories: inc, install: false)
call_dep = declare_dependency(link_with: call_lib, include_directories: inc)

gtest_dep = dependency('gtest', required: tests_opt.enabled(), disabler: true)
if tests_opt.enabled() or (tests_opt.auto() and gtest_dep.found())
  call_tests = executable('call_tests', ['call_test.cpp'], dependencies: [call_dep, gtest_dep])
  test('call_tests', call_tests)
endif
