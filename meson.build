project('CM4all klb', 'cpp', version: '0.4',
  meson_version: '>= 0.56',
  default_options: [
    'cpp_std=c++2a',
    'warning_level=3',

    'fmt:cpp_std=c++2a',
  ],
)

debug = get_option('b_ndebug') != 'true'

compiler = meson.get_compiler('cpp')

common_flags = [
  '-D_GNU_SOURCE',

  # disable boost specific code in libcommon
  '-DNO_BOOST',
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

  # clang specific warning options:
  '-Wunreachable-code-aggressive',
  '-Wused-but-marked-unused',
]

test_global_common_flags = [
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

  # clang specific warning options:
  '-Wcomma',
  '-Wheader-hygiene',
  '-Winconsistent-missing-destructor-override',
]

add_global_arguments(common_flags, language: 'cpp')

add_global_arguments(compiler.get_supported_arguments(test_global_cxxflags), language: 'cpp')
add_project_arguments(compiler.get_supported_arguments(test_cxxflags), language: 'cpp')

libsystemd = dependency('libsystemd')

inc = include_directories('src', 'libcommon/src')

libcommon_enable_boost = false

subdir('libcommon/src/util')
subdir('libcommon/src/lib/fmt')
subdir('libcommon/src/io')
subdir('libcommon/src/io/config')
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
    fmt_dep,
    io_config_dep,
    event_dep,
    avahi_dep,
    odbus_dep,
    libsystemd,
  ],
  install: true,
  install_dir: 'sbin',
)
