
/*
 * Copyright 2022 P.L. Lucas <selairi@gmail.com>
 * 
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 * 
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
 
#ifndef __TRAYDBUS_H__
#define __TRAYDBUS_H__

#include <string>
#include <functional>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <systemd/sd-bus.h>
#include <poll.h>

/*! \class TrayDBus
 *  \brief tray dbus client.
 */
class TrayDBus
{
public:
  TrayDBus();
  ~TrayDBus();

  void init();
  void finish();

  int get_fd();
  void init_struct_pollfd(struct pollfd &fds);
  void process_poll_event(struct pollfd &fds);
  std::string get_icon_title(const std::string &icon_dbus_name);
  std::string get_icon_name(const std::string &icon_dbus_name);
  void icon_activate(const std::string &icon_dbus_name, int32_t x, int32_t y);
  void icon_context_menu(const std::string &icon_dbus_name, int32_t x, int32_t y);
  bool get_icon_pixmap(const std::string &icon_dbus_name, int32_t prefered_size, int32_t *width, int32_t *height, uint8_t **bytes);
  bool get_tooltip(const std::string &icon_dbus_name, std::string &title, std::string &text);
  bool add_listener(const std::string &icon_dbus_name, const std::string &signal_name, std::function<void(const std::string &)> handler);
  bool add_listener_full(const std::string &destination, const std::string &path, const std::string interface, const std::string &signal_name, std::function<void(const std::string &)> handler);


  std::vector<std::string> get_menu_path(const std::string &icon_dbus_name);

  std::function<void(const std::string &icon_dbus_name)> add_tray_icon;
  std::function<void(const std::string &icon_dbus_name)> remove_tray_icon;


private:
  sd_bus_slot *m_slot;
  sd_bus *m_bus;
  std::string m_interface_name;
};

#endif
