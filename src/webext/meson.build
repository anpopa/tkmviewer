webext_sources = [
  'tkmv-webext.c',
 ]

webext_c_include_dirs = [
  '..',
]

webext_deps = [
  dependency('gtk4'),
  dependency('webkit2gtk-5.0'),
]

shared_library('tkmvwebext', webext_sources,
  include_directories : include_directories(webext_c_include_dirs),
  dependencies: webext_deps,
  install: true,
  install_dir : get_option('libdir') / 'tkmviewer' / 'webext'
)
