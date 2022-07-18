tkmviewer_sources = [
  'main.c',
  'tkmviewer-window.c',
  'tkmviewer-application.c',
]

tkmviewer_deps = [
  dependency('libadwaita-1', version: '>= 1.0'),
]

gnome = import('gnome')

tkmviewer_sources += gnome.compile_resources('tkmviewer-resources',
  'tkmviewer.gresource.xml',
  c_name: 'tkmviewer'
)

executable('tkmviewer', tkmviewer_sources,
  dependencies: tkmviewer_deps,
  install: true,
)
