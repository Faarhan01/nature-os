project('nature-os', ['c', 'cpp', 'vala'],
  version: '24.04.1',
  license: 'GPL-3.0-or-later',
  default_options: [
    'c_std=c17',
    'cpp_std=c++20',
    'warning_level=2',
    'optimization=3',
    'b_lto=true',
  ]
)

gnome = import('gnome')

wlroots_dep = dependency('wlroots', version: '>= 0.17')
wayland_server_dep = dependency('wayland-server')
xkbcommon_dep = dependency('xkbcommon')
gtk4_dep = dependency('gtk4', version: '>= 4.12')
gio_dep = dependency('gio-2.0')
gio_unix_dep = dependency('gio-unix-2.0')
vte_dep = dependency('vte-2.91-gtk4', required: false)
apt_pkg_dep = dependency('apt-pkg', required: false)
gstreamer_dep = dependency('gstreamer-1.0', required: false)
webkit_dep = dependency('webkitgtk-6.0', required: false)

subdir('compositor')
subdir('shell')
subdir('apps/nature-files')
subdir('apps/nature-terminal')
subdir('apps/nature-monitor')
subdir('apps/nature-settings')
subdir('apps/nature-store')
subdir('apps/nature-media')
subdir('apps/nature-browser')

install_data(
  'schemas/org.natureos.desktop.gschema.xml',
  install_dir: join_paths(get_option('datadir'), 'glib-2.0', 'schemas')
)

install_data(
  'themes/nature-gtk4/gtk.css',
  install_dir: join_paths(get_option('datadir'), 'themes', 'Nature-Botanical', 'gtk-4.0')
)

install_data(
  'systemd/nature-session.service',
  install_dir: join_paths(get_option('libdir'), 'systemd', 'user')
)

gnome.post_install(
  glib_compile_schemas: true,
  gtk_update_icon_cache: true,
)
