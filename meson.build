project(
  'vocagtk',
  'c',
  meson_version: '>= 1.3.0',
  version: '0.0.0',
  default_options: ['warning_level=3'],
)

add_global_arguments('-Wno-unused-parameter', language: 'c')

dependencies = [
  dependency('glib-2.0'),
  dependency('gtk4'),
  dependency('libcurl'),
  dependency('sqlite3'),
  dependency('yyjson'),
]

gnome = import('gnome')

res = gnome.compile_resources(
  'vocagtk-resources',
  'res/res.gresource.xml',
  source_dir: 'res',
  c_name: 'vocagtk',
  export: false,
)

inc = include_directories('include/')
src = files(
  'src/album.c',
  'src/artist.c',
  'src/db.c',
  'src/dl.c',
  'src/entry.c',
  'src/entrybox.c',
  'src/parse.c',
  'src/song.c',
  'src/ui.c',
)
main = files(
  'src/vocagtk.c',
)
testsrc = files(
  'test/test.c'
)

exe = executable(
  'vocagtk',
  dependencies: dependencies,
  include_directories: inc,
  install: true,
  sources: [main, src, res],
)

dbtest = executable(
  't',
  dependencies: dependencies,
  include_directories: inc,
  install: false,
  sources: [testsrc, src, res],
)

#test('basic', exe)
