
/*
 * Copyright 2021 P.L. Lucas <selairi@gmail.com>
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
 
#include "toplevelbutton.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <regex>
#include "settings.h"

static std::string suggested_icon_for_id(std::string id);


ToplevelButton::ToplevelButton(wayland::zwlr_foreign_toplevel_handle_v1_t toplevel_handle, wayland::seat_t seat, std::vector<ToplevelButton*> *toplevels) : Button()
{ 
  m_toplevels = toplevels;
  m_toplevel_handle = toplevel_handle;
  m_seat = seat;
  m_maximized = m_activated = m_minimized = m_fullscreen = false;

  // Listen all window events
  m_toplevel_handle.on_title() =[&](std::string title) {
    m_title = title;
  };
  m_toplevel_handle.on_app_id() =[&](std::string id) {
    m_id = id;
    std::string icon;
    // Is this icon already loaded?
    for(ToplevelButton *b : *m_toplevels) {
      if(b->m_id == m_id) {
        icon = b->getIcon();
        break;
      }
    }
    if(icon.empty()) {
      icon = suggested_icon_for_id(id);
    }
    std::cout << "                 icon " << id << " " << icon << std::endl;
    if(icon.empty())
      icon = suggested_icon_for_id(std::string("emblem-question"));
    if(icon.empty())
      init(nullptr, id);
    else
      init((char*)icon.c_str(), std::string());
    repaint_main_interface(true);
  };
  m_toplevel_handle.on_output_enter() =[&](wayland::output_t output) {
    m_output = output;
  };
  m_toplevel_handle.on_output_leave() =[&](wayland::output_t output) {
  };
  m_toplevel_handle.on_state() =[&](wayland::array_t state) {
    m_state = state;
    update_states(state);
    repaint_main_interface(true);
  };
  m_toplevel_handle.on_done() =[&]() {
  };
  m_toplevel_handle.on_closed() =[&]() {
    auto iter = std::remove(m_toplevels->begin(), m_toplevels->end(), this);
    m_toplevels->erase(iter, m_toplevels->end());
    repaint_main_interface(false);
    delete this;
  };
}

void ToplevelButton::on_mouse_clicked(int button)
{
  if(!m_activated) {
    if(m_minimized)
      m_toplevel_handle.unset_minimized();
    m_toplevel_handle.activate(m_seat);
    for(ToplevelButton *b : *m_toplevels)
      b->setSelected(false);
    setSelected(true);
  } else {
    if(m_maximized)
      m_toplevel_handle.unset_maximized();
    else
      m_toplevel_handle.set_maximized();
  }
}

void ToplevelButton::update_states(wayland::array_t toplevel_states)
{  
  m_maximized = m_activated = m_minimized = m_fullscreen = false;
  setSelected(false);
  auto states = static_cast<std::vector<wayland::zwlr_foreign_toplevel_handle_v1_state> >(toplevel_states);
  for(wayland::zwlr_foreign_toplevel_handle_v1_state state : states) {
    switch(state) {
      case wayland::zwlr_foreign_toplevel_handle_v1_state::activated:
        m_activated = true;
        setSelected(true);
        break;
      case wayland::zwlr_foreign_toplevel_handle_v1_state::maximized:
        m_maximized = true;
        break;
      case wayland::zwlr_foreign_toplevel_handle_v1_state::minimized:
        m_minimized = true;
        break;
      case wayland::zwlr_foreign_toplevel_handle_v1_state::fullscreen:
        m_fullscreen = true;
        break;
      default:
        break;
    };
  }
}

bool ToplevelButton::is_fullscreen()
{
  return m_fullscreen;
}

static std::string get_icon_from_desktop_file(std::string path, std::string id)
{
  std::string icon;
  std::filesystem::path p(path + id + std::string(".desktop"));
  std::filesystem::directory_entry entry(p);
  if(entry.exists()) {
    std::ifstream in(p.c_str());
    std::regex re("^Icon=(.*)");
    for( std::string line; std::getline(in, line); ) {
      std::smatch m;
      if(std::regex_match(line, m, re)) {
        icon = m[1];
        break;
      }
    }
  }
  return icon;
}

/** Checks desktop files to suggest an icon for application id.
 */
static std::string suggested_icon_for_id(std::string id)
{
  std::string icon;
  // Sometimes id has id.xx.xx format, the first element must be extracted
  std::string::size_type pos = id.find('.');
  if(pos != std::string::npos)
    id = id.substr(0, pos);
  icon = get_icon_from_desktop_file(std::string("/usr/share/applications/"), id);
  if(!icon.empty() && std::filesystem::directory_entry(std::filesystem::path(icon)).exists())
    return icon;
  icon = get_icon_from_desktop_file(std::string("/usr/local/share/applications/"), id);
  if(!icon.empty() && std::filesystem::directory_entry(std::filesystem::path(icon)).exists())
    return icon;
  if(!icon.empty())
    id = icon;

  icon = Icon::suggested_icon_for_id(id);
  if(!icon.empty())
    return icon;

  return std::string();
}

