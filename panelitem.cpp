
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
#include "panelitem.h"
#include "settings.h"
#include "tooltip.h"
#include <stdio.h>


PanelItem::PanelItem()
{
  m_x = m_y = m_height = m_width = 0;
  m_mouse_over = m_mouse_clicked = false;
  m_need_repaint = true;
  m_selected = false;
  m_start_position = true;
  m_timeout_msecs = -1;
  m_next_time_timeout = -1;
}

void PanelItem::set_pos(int x, int y)
{
  m_x = x;
  m_y = y;
}

void PanelItem::set_width(int width)
{
  m_width = width;
}

void PanelItem::set_height(int height)
{
  m_height = height;
}

int PanelItem::get_x()
{
  return m_x;
}

int PanelItem::get_y()
{
  return m_y;
}

int PanelItem::get_width()
{
  return m_width;
}

int PanelItem::get_height()
{
  return m_height;
}

bool PanelItem::get_selected()
{
  return m_selected;
}

bool PanelItem::is_start_pos()
{
  return m_start_position;
}
  
void PanelItem::set_start_pos(bool pos)
{
  m_start_position = pos;
}

void PanelItem::set_selected(bool selected)
{
  m_need_repaint = m_selected != selected;
  m_selected = selected;
}

bool PanelItem::need_repaint()
{
  return m_need_repaint;
}

bool PanelItem::is_in(int x, int y)
{
  if(
      m_x <= x && x <= (m_x + m_width)
      &&
      m_y <= y && y <= (m_y + m_height)
    )
    return true;
  return false;
}

void PanelItem::repaint(cairo_t *cr)
{
  Color color = Settings::get_settings()->color();
  Color background_color = Settings::get_settings()->background_color();

  if(m_mouse_clicked)
    cairo_set_source_rgba (cr, 1.0 - color.red, 1.0 - color.green, 1.0 - color.blue, 1.0);
  else if(m_mouse_over) {
    cairo_set_source_rgba (cr, background_color.red, background_color.green, background_color.blue, 1.0);
    cairo_rectangle(cr, m_x, m_y, m_width, m_height);
    cairo_fill(cr);
    cairo_set_source_rgba (cr, color.red, color.green, color.blue, 0.3);
  } else
    cairo_set_source_rgba (cr, background_color.red, background_color.green, background_color.blue, 1.0);
  cairo_rectangle(cr, m_x, m_y, m_width, m_height);
  cairo_fill(cr);

  paint(cr);

  if(m_selected) {
    cairo_set_source_rgba (cr, 1.0 - color.red, 1.0 - color.green, 1.0 - color.blue, 0.3);
    cairo_rectangle(cr, m_x, m_y, m_width, m_height);
    cairo_fill(cr);
  }

  m_need_repaint = false;
}

void PanelItem::on_mouse_leave(int x, int y, bool leave)
{
  if((!is_in(x, y) || leave) && m_mouse_over) {
    m_mouse_over = false;
    m_mouse_clicked = false;
    m_need_repaint = true;
    mouse_leave();
  }
}

void PanelItem::on_mouse_clicked(int x, int y, int button)
{
  if(is_in(x, y)) {
    m_mouse_over = true;
    m_mouse_clicked = true;
    m_need_repaint = true;
    mouse_clicked(button);
  }
}

void PanelItem::on_mouse_released(int x, int y)
{
  if(m_mouse_clicked && is_in(x, y)) {
    m_mouse_over = true;
    m_mouse_clicked = false;
    m_need_repaint = true;
    mouse_released();
  }
}

void PanelItem::on_mouse_enter(int x, int y)
{
  if(!m_mouse_over && is_in(x, y)) {
    m_mouse_over = true;
    m_mouse_clicked = false;
    m_need_repaint = true; 
    mouse_enter();
  }
}

void PanelItem::on_timeout(long now_in_msecs)
{
  if(now_in_msecs > m_next_time_timeout) {
    timeout();
    m_next_time_timeout = now_in_msecs + m_timeout_msecs;
  }
}

void PanelItem::set_timeout(int timeout_msecs)
{
  m_timeout_msecs = timeout_msecs;
  m_next_time_timeout = -1;
}

long PanelItem::next_time_timeout(long now_in_msecs)
{
  if(m_timeout_msecs < 0)
    return -1;
  if(m_next_time_timeout < 0)
    m_next_time_timeout = now_in_msecs + m_timeout_msecs;
  long module_of_timeout = m_next_time_timeout % m_timeout_msecs;
  if(module_of_timeout != 0) {
    // Try to set m_next_time_timeout to exact seconds, minutes,...
    m_next_time_timeout -= module_of_timeout;
    while(m_next_time_timeout < now_in_msecs)
      m_next_time_timeout += m_timeout_msecs;
  }
  return m_next_time_timeout;
}

void PanelItem::show_tooltip(std::string text)
{
  switch(Settings::get_settings()->panel_position()) {
    case PanelPosition::BOTTOM:
    case PanelPosition::TOP:
      ToolTip::show(text, m_x);
      break;
  }
}

// The default implementation is empty
void PanelItem::mouse_enter() { debug << "on_mouse_enter\n"; } 
void PanelItem::mouse_leave() { debug << "on_mouse_leave\n"; }
void PanelItem::mouse_clicked(int button) { debug << "on_mouse_clicked\n"; }
void PanelItem::mouse_released() { debug << "on_mouse_released\n"; }
void PanelItem::paint(cairo_t *cr) { debug << "paint\n"; }
void PanelItem::timeout() { debug << "on_timeout\n"; }
