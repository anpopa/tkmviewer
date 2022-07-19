subdir('libkplot')

tkmv_sources = [
  'main.c',
  'tkmv-window.c',
  'tkmv-application.c',
  'views/tkmv-dashboard-view.c',
  'views/tkmv-processes-view.c',
  'views/tkmv-systeminfo-view.c',
]

tkmv_c_include_dirs = [
  'libkplot',
]

kplot_dep = declare_dependency(link_with : libkplot,
include_directories : tkmv_c_include_dirs)

tkmv_deps = [
  dependency('libadwaita-1', version: '>= 1.0'),
  kplot_dep,
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
