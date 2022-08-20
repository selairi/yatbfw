
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
#include "button.h"
#include <glib.h>
#include <iostream>
#include <regex>
#include "settings.h"

Button::Button() : PanelItem()
{
  m_icon_ref = nullptr;
  init(std::string(), std::string());
}

Button::Button(const std::string & icon_path, const std::string & text) : PanelItem()
{
  m_icon_ref = nullptr;
  init(icon_path, text);
}

Button::~Button()
{
  //cairo_surface_destroy(m_icon);
  //g_object_unref(m_svg_icon);
  debug << "deleted" << std::endl;
}

void Button::init(const std::string & icon_path, const std::string & text)
{
  m_text = text;
  if(! icon_path.empty()) {
    m_icon_ref = Icon::get_icon(icon_path);
  } else {
    m_icon_ref = nullptr;
  }
}

void Button::set_text(const std::string & text)
{
  m_text = text;
  m_need_repaint = true;
}

std::string Button::get_text()
{
  return m_text;
}

void Button::set_icon(const std::string & icon_path)
{
  if(! icon_path.empty()) {
    init(icon_path, m_text);
    m_need_repaint = true;
  }
}

std::string Button::get_icon()
{
  if(m_icon_ref != nullptr)
    return m_icon_ref->get_icon_path();
  else
    return std::string();
}

void Button::draw_text(cairo_t *cr, int x_offset, int y_offset, std::string text)
{
  int text_width = m_width - x_offset;
  int text_height = m_height - y_offset;
  std::vector<std::string> lines;

  // Get lines of text
  size_t start = 0;
  size_t end = text.find("\n");
  if(end == std::string::npos)
    lines.push_back(text);
  else { 
    while (end != std::string::npos) {
      lines.push_back(text.substr(start, end - start));
      start = end + 1;
      end = text.find("\n", start);
    }
    lines.push_back(text.substr(start));
  }

  cairo_save(cr);
  cairo_rectangle(cr, m_x + x_offset, m_y + y_offset, text_width, text_height);
  cairo_clip(cr);
  Color color = Settings::get_settings()->color();
  cairo_set_source_rgba (cr, color.red, color.green, color.blue, 1.0);
  cairo_select_font_face(cr, Settings::get_settings()->font().c_str(), CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size(cr, Settings::get_settings()->font_size());
  int width, height = 0;
  for(std::string line : lines) {
    cairo_text_extents_t extents;
    cairo_text_extents(cr, line.c_str(), &extents);
    if(text_width < extents.width)
      text_width = extents.width + 6;
    height += extents.height;
  }
  int sep = 0;
  if(text_height > height)
    sep = (text_height - height) / (lines.size() + 1);
  int y = m_y + y_offset + sep;
  for(std::string line : lines) {
    cairo_text_extents_t extents;
    cairo_text_extents(cr, line.c_str(), &extents);
    width = extents.width + 6;
    cairo_move_to(cr, m_x + x_offset + (text_width - width) / 2.0, y + extents.height);
    cairo_show_text(cr, line.c_str());
    y += extents.height + sep;
  }
  cairo_restore(cr);

  if((text_height + y_offset) > m_height)
    m_height = text_height + y_offset;
  if((text_width + x_offset) > m_width)
    m_width = text_width + x_offset; 
}

void Button::paint(cairo_t *cr)
{
  int offset = 0;
  // Draws icon
  if(m_icon_ref != nullptr) {
    offset = (m_width > m_height ? m_height : m_width);
    m_icon_ref->paint(cr, m_x, m_y, offset - 1, offset - 1);
  }

  if(! m_text.empty())
    draw_text(cr, offset, 0, m_text);
}

void Button::set_tooltip(const std::string & tooltip)
{
  m_tooltip = tooltip;
}

std::string Button::get_tooltip()
{
  return m_tooltip;
}

void Button::mouse_enter()
{
  if(!m_tooltip.empty())
    show_tooltip(m_tooltip);
}
