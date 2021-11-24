project('CM4all klb', 'cpp', version: '0.1',
  meson_version: '>= 0.56',
  default_options: [
    'c_std=c11',
    'cpp_std=c++2a',
    'warning_level=3',
  ],
)

debug = get_option('b_ndebug') != 'true'

compiler = meson.get_compiler('cpp')

common_flags = [
  '-D_GNU_SOURCE',
]

test_common_flags = [
  '-Wcast-align',
  '-Wcast-qual',
  '-Wdouble-promotion',
  '-Wfloat-equal',
  '-Wmissing-declarations',
  '-Wmissing-noreturn',
  '-Wmissing-format-attribute',
  '-Wredundant-decls',
  '-Wshadow',
  '-Wundef',
  '-Wunused',
  '-Wvla',
  '-Wwrite-strings',

  '-Wno-unused-result',

  # clang specific warning options:
  '-Wunreachable-code-aggressive',
  '-Wused-but-marked-unused',
]

test_global_common_flags = [
  '-fno-omit-frame-pointer',
  '-fvisibility=hidden',
]

add_global_link_arguments(
  compiler.get_supported_link_arguments(
    # make relocations read-only (hardening)
    '-Wl,-z,relro',

    # no lazy binding, please - not worth it for a daemon
    '-Wl,-z,now',
  ),
  language: 'cpp'
)

if not debug
  test_global_common_flags += [
    '-ffunction-sections',
    '-fdata-sections',
  ]

  add_global_link_arguments(
    compiler.get_supported_link_arguments(
      '-Wl,--gc-sections',
      '-Wl,--icf=all',
    ),
    language: 'cpp'
  )
endif

test_global_cxxflags = test_global_common_flags + [
]

test_cxxflags = test_common_flags + [
  '-fno-threadsafe-statics',
  '-fmerge-all-constants',

  '-Wcomma-subscript',
  '-Wextra-semi',
  '-Wmismatched-tags',
  '-Woverloaded-virtual',
  '-Wsign-promo',
  '-Wvolatile',
  '-Wvirtual-inheritance',

  '-Wno-missing-field-initializers',

  # a vtable without a dtor is just fine
  '-Wno-non-virtual-dtor',

  # the only warnings we got from this are from formatted error
  # messages, and their truncation is harmless
  '-Wno-format-truncation',

  # clang specific warning options:
  '-Wcomma',
  '-Wheader-hygiene',
  '-Winconsistent-missing-destructor-override',
]

if compiler.get_id() == 'gcc' and compiler.version().version_compare('>=11')
  # too many bogus warnings, e.g. for std::array::size()
  test_cxxflags += '-Wno-maybe-uninitialized'
endif

add_global_arguments(common_flags, language: 'cpp')

add_global_arguments(compiler.get_supported_arguments(test_global_cxxflags), language: 'cpp')
add_project_arguments(compiler.get_supported_arguments(test_cxxflags), language: 'cpp')

libsystemd = dependency('libsystemd')

inc = include_directories('src', 'libcommon/src')

libcommon_enable_c = false
libcommon_enable_boost = false
libcommon_enable_net_log = false

subdir('libcommon/src/util')
subdir('libcommon/src/io')
subdir('libcommon/src/system')
subdir('libcommon/src/event')
subdir('libcommon/src/net')
subdir('libcommon/src/time')
subdir('libcommon/src/lib/avahi')
subdir('libcommon/src/lib/dbus')

executable(
  'cm4all-klb',
  'src/Main.cxx',
  'src/Config.cxx',
  'src/Instance.cxx',
  'src/Service.cxx',
  'src/IPVS.cxx',

  include_directories: inc,
  dependencies: [
    event_dep,
    avahi_dep,
    odbus_dep,
    libsystemd,
  ],
  install: true,
  install_dir: 'sbin',
)
