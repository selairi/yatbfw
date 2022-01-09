
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
 
#ifndef __SETTINGS_H__
#define __SETTINGS_H__

#include <string>

class Panel;

struct Color {
  float red, green, blue;
};

enum PanelPosition {
  BOTTOM, TOP
};

class Settings
{
  public:
    static Settings *get_settings();
    Settings();
    /** Load settings from file path.
     */
    void load_settings(std::string path, Panel *panel);
    static std::string home_path();
    /** Gets enviroment variable.
     */
    static std::string get_env(const char *var);

    std::string icon_theme();
    std::string font();
    int font_size();

    Color color();
    Color background_color();

    int panel_size();
    bool exclusive_zone();

    const char* cursor_theme();
    int cursor_size();

    PanelPosition panel_position();

  private:
   static Settings *m_settings; // Unique instance of settings
   std::string m_icon_theme;
   std::string m_font;
   int m_font_size;
   Color m_color;
   Color m_background_color;
   int m_panel_size;
   bool m_exclusive_zone;
   PanelPosition m_panel_position;
};

#endif
