systemd_dep = dependency('systemd')
if systemd_dep.found()
  import('fs').copyfile(
    'binarystore-migration.service',
    install: true,
    install_dir: systemd_dep.get_variable(pkgconfig: 'systemdsystemunitdir'),
  )
endif
