# yatbfw

Yes another task bar for Wayland.

This is a simple taskbar that can be used in wayland compositors that implements foreign-toplevel and layer-shell protocols. It can be configured with a simple settings file written in JSON.

# Building

Compiling requires:

- [cmake](https://github.com/Kitware/CMake)
- C++17 compiler
- [wayland-scanner++](https://github.com/NilsBrause/waylandpp)
- [libcairo](https://cairographics.org/)
- [libRSVG](https://wiki.gnome.org/Projects/LibRsvg)
- [jsoncpp](https://github.com/open-source-parsers/jsoncpp)

And run these comands:
```
mkdir build
cd build
cmake ..
make
sudo make install
```

Then edit "~/.config/yatbfw.json" to configure your taskbar.

# Settings

In the example folder you can find examples of how to configure it.
