# yatbfw

Yes another task bar for Wayland.

This is a simple taskbar that can be used in wayland compositors that implements foreign-toplevel and layer-shell protocols. It can be configured with a simple settings file written in JSON.

## Features

* Taskbar (focus on click; toggle maximize on second click; close on middleclick)
* Launchers on the left and right side
* Tooltips
* Icon theme setting
* Clock
* Style colors and size
* Top or bottom position

## Building

Dependencies are:

* [cmake](https://github.com/Kitware/CMake)
* C++17 compiler
* [wayland-scanner++](https://github.com/NilsBrause/waylandpp)
* [libcairo](https://cairographics.org/)
* [libRSVG](https://wiki.gnome.org/Projects/LibRsvg)
* [jsoncpp](https://github.com/open-source-parsers/jsoncpp)

### Archlinux

`sudo pacman -S pkgconf waylandpp cairo librsvg wayland-protocols jsoncpp`

### Debian and debian-based

`sudo apt install pkgconf wayland-scanner++ libjsoncpp-dev wayland-protocols waylandpp-dev librsvg2-dev`

Fix for  "fatal error:json/json.h: No such file or directory" create a symlink:

 `sudo ln -s /usr/include/jsoncpp/json/ /usr/include/json`

Then run these comands:
```
mkdir build
cd build
cmake ..
make
sudo make install
```
This will install in `/usr/local`. To install in `/usr` use `cmake ..  -DCMAKE_INSTALL_PREFIX=/usr` instead.

Then edit "~/.config/yatbfw.json" to configure your taskbar.

## Settings

In the example folder you can find examples of how to configure it.
