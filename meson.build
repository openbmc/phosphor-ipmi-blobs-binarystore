project(
  'phosphor-ipmi-blobs-binarystore',
  'cpp',
  version: '0.1',
  meson_version: '>=0.57.0',
  default_options: [
    'cpp_std=c++20',
    'warning_level=3',
    'werror=true',
  ]
)

blobs_dep = dependency('phosphor-ipmi-blobs')
phosphor_logging_dep = dependency('phosphor-logging')

boost_args = [
    '-DBOOST_ERROR_CODE_HEADER_ONLY',
    '-DBOOST_SYSTEM_NO_DEPRECATED',
    '-DBOOST_ALL_NO_LIB',
]

subdir('proto')
subdir('include')
subdir('src')

if not get_option('tests').disabled()
  subdir('test')
endif
