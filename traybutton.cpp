
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
 
#include <linux/input-event-codes.h>
#include "debug.h"
#include "traybutton.h"
#include "utils.h"

TrayButton::TrayButton(std::shared_ptr<TrayDBus> tray_dbus, const std::string &tray_icon_dbus_name) : PanelItem()
{
  m_icon = nullptr;
  m_tray_dbus = tray_dbus;
  m_tray_icon_dbus_name = tray_icon_dbus_name;
  m_tray_icon_name = m_tray_dbus->get_icon_name(m_tray_icon_dbus_name);
  m_tray_dbus->add_listener(m_tray_icon_dbus_name, "NewToolTip", [this](const std::string &/*arg*/) {
      printf("NewToolTip\n");
      m_need_repaint = true;
      send_repaint();
      std::string title, text;
      m_tray_dbus->get_tooltip(m_tray_icon_dbus_name, title, text);
      });
  m_tray_dbus->add_listener(m_tray_icon_dbus_name, "NewIcon", [this](const std::string &/*arg*/) {
      printf("NewIcon\n");
      m_need_repaint = true;
      send_repaint();
      });
  m_tray_dbus->add_listener(m_tray_icon_dbus_name, "NewAttentionIcon", [this](const std::string &/*arg*/) {
      printf("NewAttentionIcon\n");
      m_need_repaint = true;
      send_repaint();
      });
  m_tray_dbus->add_listener(m_tray_icon_dbus_name, "NewOverlayIcon", [this](const std::string &/*arg*/) {
      printf("NewOverlayIcon\n");
      m_need_repaint = true;
      send_repaint();
      });  
  m_tray_dbus->add_listener(m_tray_icon_dbus_name, "NewStatus", [this](const std::string &/*arg*/) {
      printf("NewStatus\n");
      m_need_repaint = true;
      send_repaint();
      });
  m_tray_dbus->add_listener(m_tray_icon_dbus_name, "NewTitle", [this](const std::string &/*arg*/) {
      printf("NewTitle\n");
      m_need_repaint = true;
      send_repaint();
      });
}

void TrayButton::set_fd(const std::vector<int> &fds)
{
  m_fds = fds;
}

void TrayButton::paint(cairo_t *cr)
{
  std::string icon_name = m_tray_dbus->get_icon_name(m_tray_icon_dbus_name);
  if(icon_name.empty()) 
    paint_pixmap(cr);
  else {
    if(icon_name != m_tray_icon_name || m_icon_ref == nullptr) {
      m_tray_icon_name = icon_name;
      m_icon_ref = Icon::get_icon(Icon::suggested_icon_for_id(m_tray_icon_name));
    }
    paint_icon_name(cr);
  }
}

void TrayButton::paint_icon_name(cairo_t *cr)
{
  int offset = 0;
  // Draws icon
  if(m_icon_ref != nullptr) {
    offset = (m_width > m_height ? m_height : m_width);
    m_icon_ref->paint(cr, (uint32_t)m_x, (uint32_t)m_y, (uint32_t)(offset - 1), (uint32_t)(offset - 1));
  }
}

void TrayButton::paint_pixmap(cairo_t *cr)
{
  int offset = (m_width > m_height ? m_height : m_width);
  int32_t w, h;
  uint8_t *bytes = nullptr;
  bool ok = m_tray_dbus->get_icon_pixmap(m_tray_icon_dbus_name, offset, &w, &h, &bytes);
  if(ok && bytes != nullptr) {
    offset--;
    if(m_icon != nullptr) {
      cairo_surface_destroy(m_icon);
      m_icon = nullptr;
    }
    static const cairo_user_data_key_t key = {0};
    m_icon = cairo_image_surface_create_for_data(
        bytes, CAIRO_FORMAT_ARGB32, w, h,
        cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, w)
    );
    cairo_surface_set_user_data(m_icon, &key, bytes, (cairo_destroy_func_t)g_free);
    if(m_icon != nullptr) {
      cairo_save(cr);
      float scale = (float)offset/(float)h;
      debug << "icon x:" << m_x << " y:" << m_y << " scale:" << scale << std::endl;
      cairo_scale(cr, (float)offset/(float)w, scale);
      cairo_set_source_surface(cr, m_icon, (double)m_x/scale, (double)m_y/scale);
      cairo_paint(cr);
      cairo_restore(cr);
    }
  }
}

void TrayButton::mouse_enter()
{
  std::string title, text;
  if(m_tray_dbus->get_tooltip(m_tray_icon_dbus_name, title, text)) {
    if(title.empty() && text.empty())
      text = m_tray_dbus->get_icon_title(m_tray_icon_dbus_name);
    else
      text = title + "\n" + text;
    show_tooltip(text);
  }
}

static std::shared_ptr<Popup> popup;

void TrayButton::mouse_clicked(int button)
{
  if(button == BTN_LEFT) 
    m_tray_dbus->icon_activate(m_tray_icon_dbus_name, m_x, m_y);
  else if(button == BTN_RIGHT) {
    //popup = new_popup();
    //popup->show(m_x);
    std::vector<std::string> args = m_tray_dbus->get_menu_path(m_tray_icon_dbus_name);
    if(args.size() < 2)
      m_tray_dbus->icon_context_menu(m_tray_icon_dbus_name, m_x, m_y);
    else {
      std::string command = "dbusmenu-cmd '" + args[0] + "' '" + args[1] + "'";
      printf("Command: %s\n", command.c_str());
      Utils::exec(command, m_fds);
    }
  }
}
