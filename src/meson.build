binarystoreblob_pre = declare_dependency(
  include_directories: blobstore_includes,
  dependencies: [
    dependency('phosphor-ipmi-blobs'),
    dependency('phosphor-logging'),
    dependency('stdplus'),
    binaryblob_nanopb_dep,
  ])

binarystoreblob_lib = library(
  'binarystoreblob',
  'binarystore.cpp',
  'sys.cpp',
  'sys_file_impl.cpp',
  'handler.cpp',
  implicit_include_directories: false,
  dependencies: binarystoreblob_pre,
  version: meson.project_version(),
  install: true)

binarystoreblob_dep = declare_dependency(
  link_with: binarystoreblob_lib,
  dependencies: binarystoreblob_pre)

shared_module(
  'binarystore',
  'main.cpp',
  implicit_include_directories: false,
  dependencies: binarystoreblob_dep,
  install: true,
  install_dir: get_option('libdir') / 'blob-ipmid')

if get_option('blobtool').allowed()
  executable(
    'blobtool',
    'blobtool.cpp',
    implicit_include_directories: false,
    dependencies: binarystoreblob_dep,
    install: true)
endif
