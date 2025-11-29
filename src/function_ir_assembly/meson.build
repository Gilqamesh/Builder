inc = include_directories('.')

function_ir_assembly_lib = library('function_ir_assembly', ['function_ir_assembly.cpp'],
  include_directories: inc,
  dependencies: [function_ir_dep],
  install: false)
function_ir_assembly_dep = declare_dependency(link_with: function_ir_assembly_lib, include_directories: inc, dependencies: [function_ir_dep])

gtest_dep = dependency('gtest', required: tests_opt.enabled(), disabler: true)
if tests_opt.enabled() or (tests_opt.auto() and gtest_dep.found())
  function_ir_assembly_tests = executable('function_ir_assembly_tests', ['function_ir_assembly_test.cpp'],
    dependencies: [function_ir_assembly_dep, gtest_dep])
  test('function_ir_assembly_tests', function_ir_assembly_tests)
endif
