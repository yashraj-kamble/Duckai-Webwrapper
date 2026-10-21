project('duckai-sidebar', 'c',
  version: '2.0.0',
  license: 'MIT',
  default_options: [
    'c_std=c11',
    'warning_level=2',
    'optimization=2',
  ]
)

gtk_dep = dependency('gtk+-3.0', version: '>= 3.22')
layer_shell_dep = dependency('gtk-layer-shell-0', version: '>= 0.6.0')
webkit_dep = dependency('webkit2gtk-4.1', version: '>= 2.38')
glib_dep = dependency('glib-2.0', version: '>= 2.50')

sources = [
  'src/main.c',
  'src/app.c',
  'src/config.c',
  'src/monitor.c',
  'src/webview.c',
  'src/ipc.c',
]

executable('duckai-sidebar',
  sources,
  dependencies: [gtk_dep, layer_shell_dep, webkit_dep, glib_dep],
  install: true,
)

install_data('data/duckai-sidebar.desktop',
  install_dir: get_option('datadir') / 'applications'
)

install_data('data/duckai-sidebar.svg',
  install_dir: get_option('datadir') / 'icons' / 'hicolor' / 'scalable' / 'apps'
)

install_data(
  ['data/duckai-sidebar.css', 'data/duckai-sidebar.conf'],
  install_dir: get_option('datadir') / 'duckai-sidebar'
)

install_data('hyprland/duckai-sidebar.conf',
  install_dir: get_option('datadir') / 'duckai-sidebar' / 'hyprland'
)
