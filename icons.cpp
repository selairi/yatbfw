
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
#include "icons.h"
#include <regex>
#include <glib.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <stdlib.h>
#include "settings.h"

std::unordered_map<std::string, std::weak_ptr<Icon> > Icon::icons;

/** Gets size from icon path. Icons paths are "8x8/", "32x32/", "scalable",...
 * This function tries to get size from path. If path is "8x8", it will return 8.
 * If path is "scalable", it will return -1.
 */
static int get_size_from_path(const std::string & path)
{
  size_t pos = path.find('x');
  if(pos != std::string::npos) {
    std::string n = path.substr(0, pos);
    try {
      return std::stoi(n);
    } catch(const std::invalid_argument& e) { }
  }
  return -1;
}

/** This class is used by sort function in order to sort
 * icon sizes.
 */
struct Sort_path_compare
{
  int panel_size;
  bool operator() (const std::string & i, const std::string & j)
  {
    int n_i = get_size_from_path(i);
    if(n_i < 0) n_i = panel_size;
    n_i -= panel_size;
    int n_j = get_size_from_path(j);
    if(n_j < 0) n_j = panel_size;
    n_j -= panel_size;
    return std::abs(n_i) < std::abs(n_j);
  }
};

struct Index_theme_file {
  std::vector<std::string> paths;
  std::vector<std::string> parent_themes;
};

static Index_theme_file read_index_theme_paths(const std::string & path, int panel_size)
{
  std::vector<std::string> paths, parents;
  std::vector<int> sizes;

  std::regex re_directories("^Directories=.*"), re_parents("^Inherits=.*");

  debug << "Reading " << path + "/index.theme" << std::endl;
  std::ifstream in(path + "/index.theme");
  for(std::string line; std::getline(in, line); ) {
    if(std::regex_match(line, re_directories)) {
      std::stringstream buff(line.substr(12));
      std::string item;
      while(getline(buff, item, ',')) {
        paths.push_back(item);
      }
    } else if(std::regex_match(line, re_parents)) {
      std::stringstream buff(line.substr(9));
      std::string item;
      while(getline(buff, item, ',')) {
        parents.push_back(item);
      }
    }
  }

  Sort_path_compare comp;
  comp.panel_size = panel_size;
  std::sort(paths.begin(), paths.end(), comp);

  Index_theme_file index_theme;
  index_theme.paths = paths;
  index_theme.parent_themes = parents;

  return index_theme;
}

static std::vector<std::string> categories_from_theme_path(const std::string & path, int panel_size)
{
  std::vector<std::string> categories;
  if(std::filesystem::directory_entry(path).exists()) {
    for(std::filesystem::directory_entry entry : std::filesystem::directory_iterator(path)) {
      if(entry.is_directory()) {
        std::string category_str = entry.path().filename().string();
        categories.push_back(category_str);
        for(std::string category : categories_from_theme_path(entry.path().string(), panel_size)) {
          categories.push_back(category_str + "/" + category);
        }
      }
    }
  }

  Sort_path_compare comp;
  comp.panel_size = panel_size;
  std::sort(categories.begin(), categories.end(), comp);

  return categories;
}

static std::string get_icon_for_theme(const std::string & path, const std::string & theme, const std::string & icon_name, int panel_size)
{
  debug << "path " << path << " theme " << theme << " icon " << icon_name << " panel_size " << panel_size << std::endl;
  std::vector<std::string> formats = {".png", ".svg"};
  Index_theme_file index_theme;
  if(theme == "hicolor")
    index_theme.paths = categories_from_theme_path(path + "/" + theme, panel_size);
  else
    index_theme= read_index_theme_paths(path + "/" + theme, panel_size);

  for(std::string category : index_theme.paths) {
    for(std::string format : formats) {
      std::string icon = path + "/" + theme + "/" + category + "/" + icon_name + format;
      // debug << "*** " << icon << std::endl;
      std::filesystem::directory_entry icon_direntry(icon);
      if(icon_direntry.exists()) {
        return icon;
      }
    }
  }

  debug << "Icon " << icon_name << " not found. Checking parent theme." << std::endl;
  for(std::string theme : index_theme.parent_themes) {
    std::string icon = get_icon_for_theme(path, theme, icon_name, panel_size);
    if(!icon.empty())
      return icon;
  }
  debug << "Icon " << icon_name << " not found." << std::endl;

  // Icon not found
  return std::string();
}

std::string Icon::suggested_icon_for_id(std::string id)
{
  {
    // Checks if id path exists
    std::filesystem::directory_entry id_direntry(id);
    if(id_direntry.exists())
      return id;
  }

  // Change id of icon to lower case (icons are saved as lower case files)
  //for(char &ch : id) {ch = std::tolower(ch);}
  std::string icon;
  Settings *settings = Settings::get_settings();
  debug << "suggested_icon_for_id " << id << " " << id + std::string("\\.(png|svg)$") << std::endl;
  
  // Paths of icon themes
  std::vector<std::string> icon_theme_paths;
  icon_theme_paths.push_back(Settings::home_path() + "/.icons/");
  icon_theme_paths.push_back(Settings::get_env("XDG_DATA_HOME") + "/");
  icon_theme_paths.push_back("/usr/share/icons/");
  icon_theme_paths.push_back("/usr/local/share/icons/");
  
  // Icons sizes
  int panel_size = settings->panel_size();

  // Themes
  std::vector<std::string> themes = {settings->icon_theme(), "hicolor"};

  std::vector<std::string> formats = {".png", ".svg"};
  
  // Checks if icon exists in icon themes  
  for(std::string icon_theme_path : icon_theme_paths) {
    for(std::string theme : themes) {
      std::string icon = get_icon_for_theme(icon_theme_path, theme, id, panel_size);
      if(!icon.empty())
        return icon;
    }
  }

  // Icon not found
  return std::string();
}


std::shared_ptr<Icon> Icon::get_icon(const std::string & path)
{
  debug << path << std::endl;
  if(path.empty())
    return nullptr;

  auto item = icons.find(path);
  if(item == icons.end()) {
    // Icon not found create a new one
    std::string icon_path = suggested_icon_for_id(path);
    if(icon_path.empty())
      return nullptr;
    // Add icon to icons map
    auto icon = std::make_shared<Icon>(path, icon_path);
    icons[path] = icon;
    return icon;
  } else {
      return item->second.lock();
  }
}

std::string Icon::get_icon_path()
{
  return m_icon_path;
}

Icon::Icon(const std::string & path, const std::string & icon_path)
{
  m_ref_count = 0;
  m_path = path;
  m_icon_path = icon_path;
  m_icon = nullptr;
  m_svg_icon = nullptr;
  if(!m_icon_path.empty()) {
    std::string str(m_icon_path);
    if(std::regex_match(str, std::regex(".*\\.[Pp][Nn][Gg]"))) {
      m_icon = cairo_image_surface_create_from_png (m_icon_path.c_str());
      if(m_icon != nullptr) {
        m_icon_width = cairo_image_surface_get_width(m_icon);
        m_icon_height = cairo_image_surface_get_height(m_icon);
      }
    } else if(std::regex_match(str, std::regex(".*\\.[Ss][Vv][Gg]"))) {
      GError *error = nullptr;
      m_svg_icon = rsvg_handle_new_from_file(m_icon_path.c_str(), &error);
      if(!m_svg_icon) {
        std::cerr << error->message << std::endl;
        g_clear_error(&error);
        exit(1);
      }
    }
  }
}

Icon::~Icon()
{
  // Delete icon from icons list
  auto item = icons.find(m_path);
  if(item != icons.end()) icons.erase(item);

  // Release cairo objects
  cairo_surface_destroy(m_icon);
  g_object_unref(m_svg_icon);
  m_icon = nullptr;
  m_svg_icon = nullptr;
  debug << "Icon " << m_path << " deleted." << std::endl;
}

void Icon::paint(cairo_t *cr, uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
  debug << " Start painting Icon " << m_path << " Path: " << m_icon_path << std::endl;
  // Draws icon
  if(m_icon != nullptr) {
    cairo_save(cr);
    float scale = (float)height/(float)m_icon_height;
    debug << "png icon x:" << x << " y:" << y << " scale:" << scale << std::endl;
    cairo_scale(cr, (float)height/(float)m_icon_width, scale);
    cairo_set_source_surface(cr, m_icon, x/scale, y/scale);
    cairo_paint(cr);
    cairo_restore(cr);
  }

  if(m_svg_icon != nullptr) {
    GError *error = nullptr;
    RsvgRectangle rect;
    rect.x = x;
    rect.y = y;
    rect.width = width;
    rect.height = height;
    debug << "SVG icon x:" << x << " y:" << y << " w:" << width << " h:" << height << std::endl; 
    if(!rsvg_handle_render_document(m_svg_icon, cr, &rect, &error)) {
      std::cerr << "[Icon::paint]" << m_path << ": " << error->message << std::endl;
      g_clear_error(&error);
      exit(1);
    }
  }
  debug << " End painting Icon " << m_path << std::endl;
}
