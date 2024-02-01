binaryblob_proto_kwargs = {
  'output': [
    '@BASENAME@.pb.h',
    '@BASENAME@.pb.cc',
  ],
  'command': [
    find_program('protoc', native: true, disabler: true, required: get_option('tests')),
    '-I@SOURCE_ROOT@/proto',
    '--cpp_out=@BUILD_ROOT@/proto',
    '@INPUT@',
  ],
}

nanopb = find_program('nanopb_generator.py', native: true, required: false)
if not nanopb.found()
  nanopb_opts = import('cmake').subproject_options()
  nanopb_opts.add_cmake_defines({'BUILD_SHARED_LIBS': 'ON'})
  nanopb_proj = import('cmake').subproject('nanopb', options: nanopb_opts)
  nanopb = find_program(meson.global_source_root() + '/subprojects/nanopb/generator/nanopb_generator.py', native: true)
  nanopb_dep = nanopb_proj.dependency('protobuf_nanopb')
else
  nanopb_dep = meson.get_compiler('cpp').find_library('protobuf-nanopb')
endif

binaryblob_nanopb_kwargs = {
  'output': [
    '@BASENAME@.pb.n.h',
    '@BASENAME@.pb.n.c',
  ],
  'command': [
    nanopb,
    '-q',
    '-s', 'packed_struct:0',
    '-H.n.h',
    '-S.n.c',
    '-I@SOURCE_ROOT@/proto',
    '-D@BUILD_ROOT@/proto',
    '@INPUT@',
  ],
}

binaryblob_proto_src = []
binaryblob_proto_hdr = []
binaryblob_nanopb_src = []
binaryblob_nanopb_hdr = []
protos = [
  'binaryblob',
]

foreach proto : protos
  tgt = custom_target(
    proto + '.pb.hcc',
    input: proto + '.proto',
    kwargs: binaryblob_proto_kwargs,
    build_by_default: false)
  binaryblob_proto_hdr += tgt[0]
  binaryblob_proto_src += tgt[1]

  tgt = custom_target(
    proto + '.pb.n.hc',
    input: proto + '.proto',
    kwargs: binaryblob_nanopb_kwargs)
  binaryblob_nanopb_hdr += tgt[0]
  binaryblob_nanopb_src += tgt[1]
endforeach

binaryblob_proto_pre = declare_dependency(
  include_directories: include_directories('.'),
  sources: binaryblob_proto_hdr,
  dependencies: [
    dependency('protobuf', disabler: true, required: get_option('tests')),
  ])

binaryblob_proto_lib = static_library(
  'binaryblob_proto',
  binaryblob_proto_src,
  implicit_include_directories: false,
  dependencies: binaryblob_proto_pre,
  build_by_default: false)

binaryblob_proto_dep = declare_dependency(
  dependencies: binaryblob_proto_pre,
  link_with: binaryblob_proto_lib)

binaryblob_nanopb_pre = declare_dependency(
  include_directories: include_directories('.'),
  sources: binaryblob_nanopb_hdr,
  dependencies: [
    nanopb_dep,
  ])

binaryblob_nanopb_lib = static_library(
  'binaryblob_nanopb',
  binaryblob_nanopb_src,
  implicit_include_directories: false,
  dependencies: binaryblob_nanopb_pre)

binaryblob_nanopb_dep = declare_dependency(
  dependencies: binaryblob_nanopb_pre,
  link_with: binaryblob_nanopb_lib)
