blobstore_pre = declare_dependency(
  include_directories: [blobstore_includes, include_directories('.')],
  dependencies : [
    blobs_dep,
    phosphor_logging_dep,
    binaryblobproto_dep
  ]
)

blobstore_lib = static_library(
  'blobstore',
  'binarystore.cpp',
  'handler.cpp',
  'sys_file_impl.cpp',
  'sys.cpp',
  src_pb,
  cpp_args: boost_args,
  implicit_include_directories: false,
  dependencies: blobstore_pre
)

blobstore_dep = declare_dependency(
  link_with: blobstore_lib,
  dependencies: blobstore_pre
)

shared_module(
  'binarystore',
  'main.cpp',
  implicit_include_directories: false,
  dependencies: [
    blobstore_dep,
  ],
  cpp_args: boost_args,
  install: true,
  install_dir: get_option('libdir') / 'ipmid-providers')

executable(
  'blobtool',
  'blobtool.cpp',
  cpp_args: boost_args,
  implicit_include_directories: false,
  dependencies: blobstore_dep,
  install: true)