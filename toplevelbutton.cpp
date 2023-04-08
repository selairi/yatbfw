
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
  
#include "debug.h"
#include "toplevelbutton.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <regex>
#include <linux/input-event-codes.h>
#include "settings.h"
#include <unordered_map>

static std::string suggested_icon_for_id(std::string id);
static std::unordered_map<std::string, std::string> init_icon_exec_map();
static std::unordered_map<std::string, std::string> icon_exec_map = init_icon_exec_map();


ToplevelButton::ToplevelButton(wayland::zwlr_foreign_toplevel_handle_v1_t toplevel_handle, wayland::seat_t seat, std::vector<std::shared_ptr<ToplevelButton> > *toplevels) : Button()
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
    for(auto b : *m_toplevels) {
      if(b->m_id == m_id && b.get() != this) {
        icon = b->get_icon();
        debug << "Icon has been already loaded for id " << id << " icon " << icon << std::endl; 
        break;
      }
    }
    if(icon.empty()) {
      icon = suggested_icon_for_id(id);
    }
    debug << "\ticon for id: " << id << " icon: >" << icon << "<" << std::endl;
    if(icon.empty() && id.find(" ") != id.npos) {
      // Id sometimes has spaces. Change id by fisrt word.
      id = id.substr(0, id.find(" "));
    }
    if(icon.empty()) {
      // Change id of icon to lower case (icons are saved as lower case files)
      std::string mod_id = id;
      for(char &ch : mod_id) {ch = (char)std::tolower(ch);}
      icon = suggested_icon_for_id(mod_id);
      debug << "\ticon for id: " << mod_id << " icon: >" << icon << "<" << std::endl;
    }
    if(icon.empty()) {
      // Sometimes id has id.xx.xx format, the first element must be extracted
      std::string::size_type pos = id.find('.');
      std::string mod_id;
      if(pos != std::string::npos)
        mod_id = id.substr(0, pos);
      icon = suggested_icon_for_id(mod_id);
      debug << "\ticon for id: " << mod_id << " icon: >" << icon << "<" << std::endl;
      if(icon.empty()) {
        // Sometimes id is in D-BUS format: xx.xx.id, where id is in PascalCase
        // Change id of icon to lower case (icons are saved as lower case files)
        for(char &ch : mod_id) {ch = (char)std::tolower(ch);}
        icon = suggested_icon_for_id(mod_id);
        debug << "\ticon for id: " << mod_id << " icon: >" << icon << "<" << std::endl;
      }
    }
    if(icon.empty()) {
      // Sometimes id has xx.xx.id format, the last element must be extracted
      std::string::size_type pos = id.find_last_of('.');
      std::string mod_id;
      if(pos != std::string::npos)
        mod_id = id.substr(pos + 1);
      icon = suggested_icon_for_id(mod_id);
      debug << "\ticon for id: " << mod_id << " icon: >" << icon << "<" << std::endl;
      if(icon.empty()) {
        // Sometimes id is in D-BUS format: xx.xx.id, where id is in PascalCase
        // Change id of icon to lower case (icons are saved as lower case files)
        for(char &ch : mod_id) {ch = (char)std::tolower(ch);}
        icon = suggested_icon_for_id(mod_id);
        debug << "\ticon for id: " << mod_id << " icon: >" << icon << "<" << std::endl;
      }
    }
    if(icon.empty() && icon_exec_map.find(id) != icon_exec_map.end()) {
      icon = icon_exec_map[id];
      debug << "\ticon for id: " << id << " icon: >" << icon << "<" << std::endl;
    }
    if(icon.empty()) {
      icon = suggested_icon_for_id(std::string("dialog-question"));
      debug << "not icon found for id " << id << std::endl;
    }
    if(icon.empty())
      init(icon, id);
    else
      init(icon, std::string());
    repaint_main_interface(true);
  };
  m_toplevel_handle.on_output_enter() =[&](wayland::output_t output) {
    m_output = output;
  };
  m_toplevel_handle.on_output_leave() =[&](wayland::output_t /*output*/) {
  };
  m_toplevel_handle.on_state() =[&](wayland::array_t state) {
    m_state = state;
    update_states(state);
    repaint_main_interface(true);
  };
  m_toplevel_handle.on_done() =[&]() {
  };
  m_toplevel_handle.on_closed() =[&]() {
    // Avoid delete this object 
    std::shared_ptr<ToplevelButton> item = *std::find_if(
        m_toplevels->begin(), m_toplevels->end(), 
        [this](std::shared_ptr<ToplevelButton> item) {
          return item.get() == this;
        }
    );
    auto iter = std::remove_if(
        m_toplevels->begin(), m_toplevels->end(), 
        [this](std::shared_ptr<ToplevelButton> item) {
          return item.get() == this;
        }
    );
    m_toplevels->erase(iter, m_toplevels->end());
    repaint_main_interface(false);
  };
}

void ToplevelButton::mouse_clicked(int button)
{
  if(!m_activated) {
    if(m_minimized)
      m_toplevel_handle.unset_minimized();
    m_toplevel_handle.activate(m_seat);
    for(auto b : *m_toplevels)
      b->set_selected(false);
    set_selected(true);
  } else {
    if(button == BTN_LEFT) {
      if(m_maximized)
        m_toplevel_handle.unset_maximized();
      else
        m_toplevel_handle.set_maximized();
    } else if(button == BTN_RIGHT) {
      if(m_minimized)
        m_toplevel_handle.unset_minimized();
      else
        m_toplevel_handle.set_minimized();
    } else if(button == BTN_MIDDLE) {
      m_toplevel_handle.close();
    }
  }
}

void ToplevelButton::update_states(wayland::array_t toplevel_states)
{  
  m_maximized = m_activated = m_minimized = m_fullscreen = false;
  set_selected(false);
  auto states = static_cast<std::vector<wayland::zwlr_foreign_toplevel_handle_v1_state> >(toplevel_states);
  for(wayland::zwlr_foreign_toplevel_handle_v1_state state : states) {
    switch(state) {
      case wayland::zwlr_foreign_toplevel_handle_v1_state::activated:
        m_activated = true;
        set_selected(true);
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
  debug << "desktop file " << p.string() << std::endl;
  std::filesystem::directory_entry entry(p);
  if(entry.exists()) {
    std::ifstream in(p.c_str());
    std::regex re("^Icon=(.*)");
    for( std::string line; std::getline(in, line); ) {
      std::smatch m;
      if(std::regex_match(line, m, re)) {
        icon = m[1];
        debug << "icon found in desktop file " << icon << std::endl;
        break;
      }
    }
  } else {
    debug << "desktop file " << p.string() << " doesn't exists." << std::endl;
  }
  return icon;
}

/** Checks desktop files to suggest an icon for application id.
 */
static std::string suggested_icon_for_id(std::string id)
{
  std::string icon;

  if(id.empty())
    return icon;
  
  std::vector<std::string> paths = { Settings::get_env("XDG_DATA_HOME") + "/applications/", "/usr/local/share/applications/", "/usr/share/applications/" };

  for(std::string path : paths) {
    icon = get_icon_from_desktop_file(path, id);
    if(!icon.empty() && std::filesystem::directory_entry(std::filesystem::path(icon)).exists())
      return icon;
    if(!icon.empty()) {
      id = icon;
      break;
    }
  }

  icon = Icon::suggested_icon_for_id(id);
  if(!icon.empty())
    return icon;

  return std::string();
}

void ToplevelButton::mouse_enter()
{
  show_tooltip(m_title);
}

// Builds a map with executable and related icon
std::unordered_map<std::string, std::string> init_icon_exec_map()
{
  std::vector<std::string> paths = { Settings::get_env("XDG_DATA_HOME") + "/applications/", "/usr/local/share/applications/", "/usr/share/applications/" };
  std::unordered_map<std::string, std::string> map; 
  for(std::string path : paths) {
    if(std::filesystem::directory_entry(std::filesystem::path(path)).exists()) {
      for(std::filesystem::directory_entry entry : std::filesystem::directory_iterator(path)) {
        if( ".desktop" == entry.path().extension()) {
          // Read content
          std::ifstream in(entry.path().c_str());
          std::regex re_icon("^Icon=(.*)");
          std::regex re_exec("^Exec=([^ ]*).*");
          std::string icon, exec;
          for( std::string line; std::getline(in, line); ) {
            std::smatch m;
            if(std::regex_match(line, m, re_icon)) {
              icon = m[1];
            } else if(std::regex_match(line, m, re_exec)) {
              exec = std::filesystem::path(m[1]).filename();
            }
          }
          map[exec] = icon;
        }
        entry.refresh();
      }
    }
  }
  return map;
}

