project(
  'phosphor-ipmi-blobs-binarystore',
  'cpp',
  version: '0.1',
  meson_version: '>=1.1.1',
  default_options: [
    'cpp_std=c++23',
    'warning_level=3',
    'werror=true',
  ]
)

subdir('proto')
subdir('include')
subdir('src')

if not get_option('tests').disabled()
  subdir('test')
endif
