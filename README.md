# yatbfw

Yes another task bar for Wayland.

This is a simple taskbar that can be used in wayland compositors that implements foreign-toplevel and layer-shell protocols. It can be configured with a simple settings file written in JSON.

# Building

To be compiled requires:

- cmake
- C++17 compiler
- wayland-scanner++
- libcairo
- libRSVG
- jsoncpp

And run these comands:
```
mkdir build
cd build
cmake ..
make
```

Then edit "~.config/yatbfw.json" to configure your taskbar.
