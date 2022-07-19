libkplot_sources = [
  'array.c',
  'border.c',
  'bucket.c',
  'buffer.c',
  'colours.c',
  'draw.c',
  'extern.h',
  'grid.c',
  'hist.c',
  'kdata.c',
  'kplot.c',
  'kplot.h',
  'label.c',
  'margin.c',
  'mean.c',
  'plotctx.c',
  'reallocarray.c',
  'stddev.c',
  'tic.c',
  'vector.c'
]

cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required : false)

libkplot_deps = [
  dependency('cairo'),
  m_dep
]

libkplot = static_library('kplot',
  libkplot_sources,
  dependencies: libkplot_deps,
  install : false)
