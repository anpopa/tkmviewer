libtkm_sources = [
  'tkm-settings.c',
  'tkm-action.c',
  'tkm-context.c',
  'tkm-entrypool.c',
  'tkm-session-entry.c',
  'tkm-cpustat-entry.c',
  'tkm-meminfo-entry.c',
  'tkm-procevent-entry.c',
  'tkm-pressure-entry.c',
  'tkm-buddyinfo-entry.c',
  'tkm-wireless-entry.c',
  'tkm-diskstat-entry.c',
  'tkm-procinfo-entry.c',
  'tkm-procacct-entry.c',
  'tkm-ctxinfo-entry.c',
  'tkm-task.c',
  'tkm-taskpool.c',
]

libtkm_c_include_dirs = [
  '.',
]

libtkm_deps = [
  dependency('glib-2.0', version : '>=2.58'),
  dependency('sqlite3')
]

libtkm = static_library('libtkm',
  libtkm_sources,
  include_directories : include_directories(libtkm_c_include_dirs),
  dependencies: libtkm_deps,
  install : false)
