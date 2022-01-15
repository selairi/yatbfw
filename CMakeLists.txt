cmake_minimum_required(VERSION 3.21) 
project (yatbfw VERSION 0.1)

set(CMAKE_CXX_STANDARD 17)

include(FindPkgConfig)
find_library(LIBRT rt)
mark_as_advanced(LIBRT)

pkg_check_modules(RSVG REQUIRED librsvg-2.0>=2.46)

configure_file(configure.h.in configure.h)

list(APPEND EXTRA_INCLUDES "${PROJECT_SOURCE_DIR}/protocols")

set(SHARE_FILES
    example/yatbfw.json
)


message(STATUS "Build protocols")
execute_process(COMMAND bash build-protocols.sh
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/protocols
  RESULT_VARIABLE protocols_result
  OUTPUT_VARIABLE protocols_output)
message(STATUS "Protocols [${protocols_result}]: ${protocols_output}")

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
  debug.cpp
  protocols/layer-shell.cpp
  protocols/toplevel.cpp
)

include_directories(${RSVG_INCLUDE_DIRS} ${EXTRA_INCLUDES} "${PROJECT_BINARY_DIR}")
target_link_libraries(yatbfw wayland-client++ wayland-client-extra++ wayland-cursor++ cairo ${RSVG_LIBRARIES} jsoncpp)
if(LIBRT)
  target_link_libraries(yatbfw "${LIBRT}")
endif()

install(TARGETS yatbfw DESTINATION bin)
install(FILES ${SHARE_FILES} DESTINATION share/yatbfw)
