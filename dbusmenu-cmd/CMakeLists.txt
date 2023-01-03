cmake_minimum_required(VERSION 3.16)

project(dbusmenu-cmd VERSION 1.0.0 LANGUAGES CXX)

find_package(dbusmenu-qt5 REQUIRED)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 COMPONENTS Core Gui Widgets DBus)

add_executable(dbusmenu-cmd
    main.cpp
)

target_link_libraries(dbusmenu-cmd PRIVATE Qt5::Core Qt5::Gui Qt5::Widgets Qt::DBus dbusmenu-qt5)

install(TARGETS dbusmenu-cmd DESTINATION bin)
