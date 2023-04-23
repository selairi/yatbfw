
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
 
#ifndef __TRAYBUTTON_H__
#define __TRAYBUTTON_H__

#include <string>
#include <functional>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <systemd/sd-bus.h>
#include <memory>
#include <vector>
#include "panelitem.h"
#include "traydbus.h"
#include "icons.h"
#include "popup.h"

/*! \class TrayButton
 *  \brief tray item to add to panel.
 *
 *  This is a battery control item. It shows battery capacity.
 */
class TrayButton : public PanelItem
{
public:
  TrayButton(std::shared_ptr<TrayDBus> tray_dbus, const std::string &tray_icon);
  virtual ~TrayButton();

  virtual void mouse_clicked(int button) override;
  virtual void mouse_enter() override;
  virtual void paint(cairo_t *cr) override;

  std::function<void()> send_repaint;
  std::function<std::shared_ptr<Popup>()> new_popup;

  void set_fd(const std::vector<int> &fds);
private:
  std::shared_ptr<TrayDBus> m_tray_dbus;
  std::string m_tray_icon_dbus_name;
  std::string m_tray_icon_name;
  std::shared_ptr<Icon> m_icon_ref;
  cairo_surface_t *m_icon;

  std::vector<int> m_fds;

  void paint_pixmap(cairo_t *cr);
  void paint_icon_name(cairo_t *cr);
};

#endif
