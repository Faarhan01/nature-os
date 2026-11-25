executable('nature-compositor',
  files('src/main.c', 'src/render_scheduler.c'),
  dependencies: [wlroots_dep, wayland_server_dep, xkbcommon_dep],
  c_args: ['-DWLR_USE_UNSTABLE'],
  install: true,
)
