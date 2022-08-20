
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
#include "battery.h"
#include <filesystem>
#include <fstream>
#include <iostream>


static std::string read_line(const char *path)
{
  std::string text;
  
  std::ifstream in(path);
  std::getline(in, text);
  in.close();

  return text;
}

static std::string read_line(std::string path)
{
  return read_line(path.c_str());
}

static std::string get_battery_path()
{
  std::filesystem::path path("/sys/class/power_supply/");
  std::filesystem::directory_entry entry_path(path);
  if(entry_path.exists()) {
    for(std::filesystem::directory_entry entry : std::filesystem::directory_iterator(path)) {
      std::string type = read_line(entry.path().string() + std::string("/type"));
      if(type == "Battery") {
        return std::string(entry.path().string());
      }
    }
  }
  return std::string();
}

std::string get_power()
{
  std::string battery_path = get_battery_path();

  if(battery_path.empty())
    return std::string();

  std::string text, line;
  
  // Read capacity
  text = read_line(battery_path + "/capacity");

  // Charging or discharging
  line = read_line(battery_path + "/status");

  if(line != "Discharging")
    text = "+ " + text;
  return text + "%";
}

Battery::Battery(
   const std::string & icon_battery_full, // Battery level 100% - 80%
   const std::string & icon_battery_good,    // 80% - 60%
   const std::string & icon_battery_medium,  // 60% - 40%
   const std::string & icon_battery_low,     // 40% - 20%
   const std::string & icon_battery_empty,    // 20% - 0%
   const std::string & icon_battery_charging,
   const std::string & icon_battery_charged,
   bool no_text
) : ButtonRunCommand()
{
  m_icon_battery_full     = icon_battery_full;
  m_icon_battery_good     = icon_battery_good;
  m_icon_battery_medium   = icon_battery_medium;
  m_icon_battery_low      = icon_battery_low;
  m_icon_battery_empty    = icon_battery_empty;
  m_icon_battery_charging = icon_battery_charging;
  m_icon_battery_charged  = icon_battery_charged;

  m_no_text = no_text;
  m_level = 0;

  update_battery_level();
  set_timeout(60000); // Battery is updated each 60 seconds
}

void Battery::update_battery_level()
{
  std::string battery_path = get_battery_path();
  std::string power, status;
  int level = 0;
  
  if(battery_path.empty())
    return;
  
  // Read capacity
  power = read_line(battery_path + "/capacity");
  if(! power.empty())
    level = std::stoi(power);

  // Charging or discharging
  status = read_line(battery_path + "/status");

  if(status != "Discharging")
    power = "+" + power;
  else
    level = -level;
  power += "%";
  
  if(level != m_level) {
    m_level = level;
    set_text(m_no_text ? "" : power);
    if(status == "Discharging") {
      level = -level;
      if(level > 80)                     set_icon(m_icon_battery_full);
      else if(level <= 80 && level > 60) set_icon(m_icon_battery_good);
      else if(level <= 60 && level > 40) set_icon(m_icon_battery_medium);
      else if(level <= 40 && level > 20) set_icon(m_icon_battery_low);
      else if(level <= 20)               set_icon(m_icon_battery_empty);
    } else {
      if(level >= 100) set_icon(m_icon_battery_charged);
      else             set_icon(m_icon_battery_charging);
    }
  }
}

void Battery::timeout()
{
  update_battery_level();
  send_repaint();
}

void Battery::mouse_enter()
{
  std::string battery_path = get_battery_path();
  if(battery_path.empty())
    return;

  std::string text;

  try {
    std::string voltage = read_line(battery_path + "/voltage_now");
    std::string current = read_line(battery_path + "/current_now");
    std::string capacity = read_line(battery_path + "/capacity");
    std::string status = read_line(battery_path + "/status");
    std::string charge_full_design = read_line(battery_path + "/charge_full_design");
    std::string charge_full = read_line(battery_path + "/charge_full");
    std::string cycle_count = read_line(battery_path + "/cycle_count");
    std::string technology = read_line(battery_path + "/technology");

    if(!status.empty())
      text += "Battery: " + status + std::string("\n");
    if(!capacity.empty())
      text += "Capacity: " +capacity + std::string("%\n");
    if(!voltage.empty() && !current.empty()) {
      int v = std::stoi(voltage);
      int i = std::stoi(current);
      text += std::string("Power: ") + std::to_string((float)v*(float)i/1e12) + std::string("W\n");
    }
    if(!technology.empty())
      text += std::string("Technology: ") + technology + std::string("\n");
    if(!charge_full.empty() && !charge_full_design.empty()) {
      float cf = (float)std::stoi(charge_full);
      float cfd = (float)std::stoi(charge_full_design);
      int degradation = 100 - (int)(100.0*cf/cfd);
      text += std::string("Degradation: ") + std::to_string(degradation) + std::string("%\n");
    }
    if(!cycle_count.empty())
      text += std::string("Cycle count: ") + cycle_count;

    show_tooltip(text);
  } catch (std::string &err) {
    debug_error << err;
  }
}
