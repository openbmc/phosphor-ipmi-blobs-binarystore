# Copyright 2021 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

protoc = find_program('protoc')

protos = [
  'binaryblob.proto',
]

src_pb = []
foreach proto : protos
  src_pb += custom_target(
    'src_pb' + proto,
    output: ['@BASENAME@.pb.cc', '@BASENAME@.pb.h'],
    input: proto,
    command: [
      protoc,
      '--proto_path=@CURRENT_SOURCE_DIR@',
      '--cpp_out=@OUTDIR@',
      '--experimental_allow_proto3_optional',
      '@INPUT@'
    ]
  )
endforeach


binaryblobproto_pre = declare_dependency(
  include_directories: include_directories('.'),
  dependencies: dependency('protobuf')
)

binaryblobproto_lib = static_library(
  'binaryblob_proto',
  src_pb,
  implicit_include_directories: false,
  dependencies: binaryblobproto_pre
)

binaryblobproto_dep = declare_dependency(
  dependencies: binaryblobproto_pre,
  link_with: binaryblobproto_lib
)
