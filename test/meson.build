gtest = dependency('gtest', main: true, disabler: true, required: get_option('tests'))
gmock = dependency('gmock', disabler: true, required: get_option('tests'))

test_dep = declare_dependency(
  include_directories: include_directories('.'),
  dependencies: [gtest, gmock])

tests = [
  'binarystore',
  'parse_config',
  'sys_file',
  'handler',
  'handler_open',
  'handler_readwrite',
  'handler_commit',
  'handler_stat',
]

foreach t : tests
  test(
    t,
    executable(
      t.underscorify(),
      t + '_unittest.cpp',
      implicit_include_directories: false,
      dependencies: [binarystoreblob_dep, test_dep]))
endforeach
