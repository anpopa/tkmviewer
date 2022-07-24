subdir('libkplot')
subdir('libtkm')

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
