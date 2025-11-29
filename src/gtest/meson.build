gtest_support_deps = [
  dependency('gtest', required: true),
  dependency('gtest_main', required: true),
]

gtest_support = declare_dependency(dependencies: gtest_support_deps)
meson.override_dependency('gtest_support', gtest_support)
