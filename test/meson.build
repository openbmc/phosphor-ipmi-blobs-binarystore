gtest = dependency('gtest', main: true, disabler: true, required: false)
gmock = dependency('gmock', disabler: true, required: false)
if not gtest.found() or not gmock.found()
  gtest_opt = import('cmake').subproject_options()
  gtest_opt.append_compile_args('c++', ['-DCMAKE_CXX_FLAGS=-Wno-pedantic'])
  gtest_proj = cmake.subproject('googletest', options: gtest_opt, required: false)
  fmt_depj.dependency('fmt')
   if gtest_proj.found()
    gtest = declare_dependency(
      dependencies: [
        dependency('threads'),
        gtest_proj.dependency('gtest'),
        gtest_proj.dependency('gtest_main'),
      ])
    gmock = gtest_proj.dependency('gmock')
  endif
endif


blobstore_tests = [
  'handler_commit',
  'handler_open',
  'handler_readwrite',
  'handler_stat',
  'handler',
  'binarystore',
  'parse_config',
  'sys_file',
]

foreach t : blobstore_tests
  test(
    t,
    executable(
      t.underscorify(), t + '_unittest.cpp',
      build_by_default: false,
      implicit_include_directories: false,
      include_directories: [blobstore_includes, include_directories('.')],
      dependencies: [blobstore_dep, gtest, gmock])
    )
endforeach
