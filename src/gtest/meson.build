project('gtest', 'cpp')

gtest_dep = declare_dependency(
  dependencies: [
    dependency('gtest', required: true),
    dependency('gtest_main', required: true),
  ],
)

meson.override_dependency('gtest', gtest_dep)
meson.override_dependency('gtest_support', gtest_dep)
