project (yatbfw)
cmake_minimum_required(VERSION 3.21) 

set(CMAKE_CXX_STANDARD 17)

include(FindPkgConfig)
find_library(LIBRT rt)
mark_as_advanced(LIBRT)

pkg_check_modules(RSVG REQUIRED librsvg-2.0>=2.46)

#add_subdirectory(protocols)
#list(APPEND EXTRA_LIBS Wayland_protocols)
list(APPEND EXTRA_INCLUDES "${PROJECT_SOURCE_DIR}/protocols")

add_executable(yatbfw 
  main.cpp 
  panel.cpp
  panelitem.cpp
  button.cpp
  buttonruncommand.cpp
  toplevelbutton.cpp
  clock.cpp
  battery.cpp
  settings.cpp
  utils.cpp
  icons.cpp
  protocols/layer-shell.cpp
  protocols/toplevel.cpp
)

include_directories(${RSVG_INCLUDE_DIRS} ${EXTRA_INCLUDES})
#target_link_libraries(yatbfw wayland-client++ wayland-client-extra++ wayland-cursor++ cairo ${RSVG_LIBRARIES} jsoncpp ${EXTRA_LIBS})
target_link_libraries(yatbfw wayland-client++ wayland-client-extra++ wayland-cursor++ cairo ${RSVG_LIBRARIES} jsoncpp)
if(LIBRT)
  target_link_libraries(yatbfw "${LIBRT}")
endif()
