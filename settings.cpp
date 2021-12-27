
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
 
#include "settings.h"
#include "panel.h"
#include "utils.h"
#include <json/json.h>
#include <fstream>
#include <iostream>
#include <stdlib.h>

Settings *Settings::m_settings = nullptr;

Settings *Settings::get_settings()
{
  if(m_settings == nullptr)
    m_settings = new Settings();
  return m_settings;
}

std::string Settings::home_path()
{
  const char *home = getenv("HOME");
  return std::string(home);
}

std::string Settings::get_env(const char * var)
{
  const char *v = getenv(var);
  return std::string(v);
}

std::string Settings::icon_theme()
{
  return m_icon_theme;
}

int Settings::font_size()
{
  return m_font_size;
}

std::string Settings::font()
{
  return m_font;
}

Color Settings::color()
{
  return m_color;
}

Color Settings::background_color()
{
  return m_background_color;
}

const char *Settings::cursor_theme()
{
  //const char *theme = "default";
  return "default";
}

int Settings::cursor_size()
{
  int size = 16;
  std::string env = get_env("XCURSOR_SIZE");
  if(! env.empty())
    size = atoi(env.c_str());

  return size;
}

int Settings::panel_size()
{
  return m_panel_size;
}

PanelPosition Settings::panel_position()
{
  return m_panel_position;
}

Settings::Settings()
{
  m_icon_theme = "hicolor"; 
  m_font = "Helvetica";
  m_font_size = 20;
  m_color = {.red = 0.0, .green = 0.0, .blue = 0.0};
  m_background_color = {.red = 0.9, .green = 0.9, .blue = 1.0};
  m_panel_size = 33;
  m_panel_position = PanelPosition::BOTTOM;
}

static void load_items(const Json::Value &items, Panel *panel, bool start_pos)
{
  for(Json::Value item : items) {
    if(item.get("type", "").asString() == std::string("launcher")) {
      std::string icon = item.get("icon", "").asString();
      std::string exec = item.get("exec", "").asString();
      std::string text = item.get("text", "").asString();
      panel->addLauncher(icon, text, exec, start_pos);
    } else if(item.get("type", "").asString() == std::string("clock")) {
      std::string icon = item.get("icon", "").asString();
      std::string exec = item.get("exec", "").asString();
      std::string format = item.get("time_format", "").asString();
      panel->addClock(icon, format, exec, start_pos);
    } else if(item.get("type", "").asString() == std::string("battery")) {
      std::string exec = item.get("exec", "").asString();
      panel->addBattery(exec, start_pos);
    }
  }
}

void Settings::load_settings(std::string path, Panel *panel)
{
  std::cout << "[Settings::load_settings] loading... " << path << std::endl; 

  Json::Value json;
  std::ifstream json_file(path);

  json_file >> json;

  m_icon_theme = json.get("icon_theme", "").asString();
  if(m_icon_theme.empty()) {
    // Load default icon theme
    std::string theme = Utils::read_command("gsettings get org.gnome.desktop.interface icon-theme");
    theme = theme.substr(1, theme.size() - 1);
    m_icon_theme = std::string("/usr/share/icons/") + theme;
  }

  std::cout << "icon_theme "  << m_icon_theme << std::endl;

  m_font = json.get("font", "Helvetica").asString();
  m_font_size = json.get("font_size", "20").asInt();

  m_panel_size = json.get("size", 33).asInt();
  if(json.get("position", "").asString() == "top")
    m_panel_position = PanelPosition::TOP;
  else
    m_panel_position = PanelPosition::BOTTOM;

  const Json::Value color = json["color"];
  m_color.red = color[0].asFloat();
  m_color.green = color[1].asFloat();
  m_color.blue = color[2].asFloat();

  const Json::Value background_color = json["background_color"];
  m_background_color.red = background_color[0].asFloat();
  m_background_color.green = background_color[1].asFloat();
  m_background_color.blue = background_color[2].asFloat();

  std::cout << "background_color " << m_background_color.red << " " << m_background_color.green << " " << m_background_color.blue << std::endl;

  const Json::Value start_items = json["start_items"];
  load_items(start_items, panel, true);

  const Json::Value end_items = json["end_items"];
  load_items(end_items, panel, false);
}
