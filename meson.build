project(
    'phosphor-hostEvent-monitor',
    'cpp',
    version: '1.0',
    default_options: [
        'cpp_std=c++17',
    ],
)

executable(
    'hostEvent-monitor',
    [
        'hostEventMonitor.cpp',
    ],
    dependencies: [
        dependency('phosphor-logging'),
        dependency('sdbusplus'),
        dependency('phosphor-dbus-interfaces'),
        dependency('sdeventplus'),
        dependency('threads'),
    ],
    install: true,
    install_dir: get_option('bindir')
)

install_data(sources : 'bmc_hostEvent_config.json', install_dir : '/etc/hostEventMon')

conf_data = configuration_data()
conf_data.set('HOST_EVENT_CONFIG_FILE', '"/etc/hostEventMon/bmc_hostEvent_config.json"')
conf_data.set('HOST_EVENT_BUS_NAME', '"xyz.openbmc_project.HostEventMon"')
conf_data.set('HOST_EVENT_SENSOR_PATH', '"/xyz/openbmc_project/sensors/"')

configure_file(output : 'config.h',
               configuration : conf_data)

systemd = dependency('systemd')
conf_data = configuration_data()
conf_data.set('bindir', get_option('prefix') / get_option('bindir'))
configure_file(
  input: 'phosphor-hostEvent-monitor.service.in',
  output:'phosphor-hostEvent-monitor.service',
  configuration: conf_data,
  install: true,
  install_dir: systemd.get_pkgconfig_variable('systemdsystemunitdir'))
