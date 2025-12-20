project(
  'vocagtk',
  'c',
  meson_version: '>= 1.3.0',
  version: '0.0.0',
  default_options: ['warning_level=3'],
)

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
  'src/data.c',
  'src/entry.c',
  'src/entrybox.c',
  'src/song.c',
  'src/vocagtk.c',
)

exe = executable(
  'vocagtk',
  dependencies: dependencies,
  include_directories: inc,
  install: true,
  sources: [src, res],
)

#test('basic', exe)
