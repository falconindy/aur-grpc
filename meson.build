project('aur-grpc', 'cpp',
        version : '0',
        license : 'MIT',
        meson_version : '>=0.50',
        default_options : [
          'cpp_std=c++17',
          'warning_level=2',
        ])

cpp = meson.get_compiler('cpp')

libgrpcpp = dependency('grpc++')
libprotobuf = dependency('protobuf')
libalpm = dependency('libalpm')
libsystemd = dependency('libsystemd')
gtest = dependency('gtest_main',
                   version : '>=1.10.0',
                   disabler : true)
gmock = dependency('gmock',
                   version : '>=1.10.0',
                   disabler : true)
libgrpcpp_reflection = cpp.find_library('grpc++_reflection')

abseil_proj = subproject(
  'abseil-cpp',
  default_options : [
    'default_library=static',
  ])
abseil = declare_dependency(
  dependencies : [
    abseil_proj.get_variable('absl_container_dep'),
    abseil_proj.get_variable('absl_strings_dep'),
  ],
  include_directories : [
    abseil_proj.get_variable('absl_include_dir'),
  ])

protoc = find_program('protoc')
grpc_cpp_plugin = find_program('grpc_cpp_plugin')

protoc_gen = generator(
  protoc,
  output : [
    '@BASENAME@.grpc.pb.cc',
    '@BASENAME@.grpc.pb.h',
    '@BASENAME@.pb.cc',
    '@BASENAME@.pb.h',
  ],
  arguments : [
    '--proto_path=@CURRENT_SOURCE_DIR@/src/proto',
    '--cpp_out=@BUILD_DIR@',
    '--grpc_out=@BUILD_DIR@',
    '--plugin=protoc-gen-grpc=@0@'.format(grpc_cpp_plugin.path()),
    '@INPUT@',
  ]
)

aur_v1_proto = declare_dependency(
  sources : protoc_gen.process('src/proto/aur_v1.proto'),
  dependencies : [
    libgrpcpp,
    libprotobuf,
  ],
)

aur_internal_proto = declare_dependency(
  sources : protoc_gen.process('src/proto/aur_internal.proto'),
  dependencies : [
    libprotobuf,
  ],
)

storage = declare_dependency(
  link_with : [
    static_library(
      'storage',
      files('''
        src/storage/file_io.cc src/storage/file_io.hh
        src/storage/filesystem_storage.cc src/storage/filesystem_storage.hh
        src/storage/storage.hh
      '''.split()),
      include_directories : [
        'src',
      ],
      dependencies : [
        abseil,
        libprotobuf,
      ],
    ),
  ],
  include_directories : ['src'])

service_internal = declare_dependency(
  link_with : [
    static_library(
      'service_internal',
      files('''
        src/service/internal/service_impl.hh src/service/internal/service_impl.cc
        src/service/internal/package_index.hh src/service/internal/package_index.cc
        src/service/internal/parsed_dependency.hh src/service/internal/parsed_dependency.cc
      '''.split()),
      include_directories : [
        'src',
      ],
      dependencies : [
        aur_internal_proto,
        abseil,
        libgrpcpp,
        libprotobuf,
        libalpm,
      ]),
  ],
  dependencies : [
    abseil,
    aur_internal_proto,
    libgrpcpp,
  ],
  include_directories : [
    'src',
  ])

service_v1 = declare_dependency(
  link_with : [
    static_library(
      'service_v1',
      files('''
        src/service/v1/conversions.hh src/service/v1/conversions.cc
        src/service/v1/service.hh src/service/v1/service.cc
      '''.split()),
      include_directories : [
        'src',
      ],
      dependencies : [
        aur_v1_proto,
        aur_internal_proto,
        libgrpcpp,
        libprotobuf,
        service_internal,
      ]),
  ],
  dependencies : [
    service_internal,
  ],
  include_directories : [
    'src',
  ])

server = executable(
  'server',
  files('''
    src/server/server.hh src/server/server.cc
    src/server/main.cc
  '''.split()),
  include_directories : [
    'src',
  ],
  dependencies : [
    aur_v1_proto,
    aur_internal_proto,
    libgrpcpp_reflection,
    libsystemd,
    service_v1,
    storage,
  ])

client = executable(
  'client',
  files('''
    src/client/client.cc src/client/client.hh
    src/client/main.cc
  '''.split()),
  include_directories : [
    'src',
  ],
  dependencies : [
    aur_v1_proto,
  ])

executable(
  'json_to_protobuf',
  files('''
    tools/json_to_protobuf.cc
  '''.split()),
  protoc_gen.process('src/proto/legacy.proto'),
  include_directories : [
    'src',
  ],
  dependencies : [
    abseil,
    aur_internal_proto,
    storage,
  ],
  install : false)

test(
  'service_v1_test',
  executable(
    'service_v1_test',
    files('''
      src/service/v1/conversions_test.cc
    '''.split()),
    include_directories : [
      'src'
    ],
    dependencies : [gtest, gmock, service_v1, aur_v1_proto, aur_internal_proto]))

test(
  'service_internal_test',
  executable(
    'service_internal_test',
    files('''
      src/service/internal/service_impl_test.cc
      src/service/internal/package_index_test.cc
      src/service/internal/parsed_dependency_test.cc
      src/storage/inmemory_storage.hh src/storage/inmemory_storage.cc
    '''.split()),
    include_directories : [
      'src'
    ],
    dependencies : [abseil, gtest, gmock, storage, service_internal]))

run_target(
  'fmt',
  command : [
    join_paths(meson.source_root(), 'build-aux/source-format')
  ])
