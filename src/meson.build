subdir('libkplot')
subdir('libtkm')
subdir('webext')

tkmv_types_data = configuration_data()
tkmv_types_data.set('version', meson.project_version())
tkmv_types_data.set('install_prefix', get_option('prefix'))
tkmv_types_data.set('lib_install_path', get_option('libdir') / 'tkmviewer')
tkmv_types_data.set('build_path', meson.project_build_root())
configure_file(input : 'tkmv-types.h.in',
  output : 'tkmv-types.h',
  configuration : tkmv_types_data)

tkmv_sources = [
  'main.c',
  'tkmv-window.c',
  'tkmv-application.c',
  'tkmv-preferences-window.c',
  'model/tkmv-settings.c',
  'model/tkmv-settings-recent-file.c',
  'views/tkmv-dashboard-view.c',
  'views/tkmv-processes-view.c',
  'views/tkmv-systeminfo-view.c',
]

tkmv_c_include_dirs = [
  'model',
  'libkplot',
  'libtkm',
]

libkplot_dep = declare_dependency(link_with : libkplot,
include_directories : tkmv_c_include_dirs)

libtkm_dep = declare_dependency(link_with : libtkm,
include_directories : tkmv_c_include_dirs)

tkmv_deps = [
  dependency('libadwaita-1', version: '>= 1.0'),
  dependency('webkit2gtk-5.0'),
  libkplot_dep,
  libtkm_dep
]

gnome = import('gnome')

tkmv_sources += gnome.compile_resources('tkmv-resources',
  'tkmviewer.gresource.xml',
  c_name: 'tkmviewer'
)

executable('tkmviewer', tkmv_sources,
  dependencies: tkmv_deps,
  install: true,
)
