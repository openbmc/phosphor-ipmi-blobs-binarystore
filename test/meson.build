gtest = dependency('gtest', main: true, disabler: true, required: get_option('tests'))
gmock = dependency('gmock', disabler: true, required: get_option('tests'))

tests = [
  'binarystore_unittest',
  'parse_config_unittest',
  'sys_file_unittest',
  'handler_unittest',
  'handler_open_unittest',
  'handler_readwrite_unittest',
  'handler_commit_unittest',
  'handler_stat_unittest',
]

foreach t : tests
  test(
    t,
    executable(
      t.underscorify(),
      t + '.cpp',
      implicit_include_directories: false,
      dependencies: [binarystoreblob_dep, binaryblob_proto_dep, gtest, gmock]))
endforeach
