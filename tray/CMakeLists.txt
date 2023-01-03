project (status-notify-watcher-daemon VERSION 0.1)

set(CMAKE_CXX_STANDARD 17)

include(FindPkgConfig)

pkg_check_modules(SYSTEMD REQUIRED libsystemd>=0.34)


add_executable(status-notify-watcher-daemon 
  dbus-tray.cpp 
)

target_link_libraries(status-notify-watcher-daemon ${SYSTEMD_LIBRARIES})

install(TARGETS status-notify-watcher-daemon DESTINATION bin)
