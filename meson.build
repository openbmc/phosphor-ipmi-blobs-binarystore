project(
  'phosphor-ipmi-blobs-binarystore',
  'cpp',
  version: '0.1',
  meson_version: '>=0.51.0',
  default_options: [
    'cpp_std=c++17',
    'warning_level=3',
    'werror=true',
  ])


cpp = meson.get_compiler('cpp')
cpp.has_header('boost/endian/arithmetic.hpp')
cpp.has_header('nlohmann/json.hpp')

protobuf_dep = dependency('protobuf')
ipmi_blob_dep = dependency('phosphor-ipmi-blobs')
phosphor_logging_dep = dependency('phosphor-logging')


proto = custom_target(
  'proto',
  command: [
    find_program('protoc', native: true),
    '--proto_path=@CURRENT_SOURCE_DIR@',
    '--cpp_out=@OUTDIR@',
    '@INPUT@'
  ],
  output: [
    'binaryblob.pb.cc',
    'binaryblob.pb.h',
  ],
  input: 'binaryblob.proto')
proto_h = proto[1]

binarystoreblob_pre = declare_dependency(
  sources: proto_h,
  include_directories: include_directories('.'),
  dependencies: [
    protobuf_dep,
    ipmi_blob_dep,
    phosphor_logging_dep,
  ])

binarystoreblob_lib = static_library(
  'binarystore',
  'binarystore.cpp',
  'sys.cpp',
  'sys_file_impl.cpp',
  'handler.cpp',
  proto,
  implicit_include_directories: false,
  dependencies: binarystoreblob_pre)

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

if not get_option('tests').disabled()
  subdir('test')
endif
