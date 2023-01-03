
/*
 * Copyright 2023 P.L. Lucas <selairi@gmail.com>
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
#include "popup.h"
#include "settings.h"
#include "utils.h"
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>
#include <iostream>
#include <array>
#include <memory>
#include <sstream>
#include <ctime>
#include <algorithm>
#include <random>
#include "button.h" // FIXME: Erase this line after testing

#define popup_margin 16

using namespace wayland;

#include "shared_mem.hpp"

Popup::Popup(compositor_t *compositor, display_t *display, xdg_wm_base_t *xdg_wm_base, shm_t *shm, zwlr_layer_surface_v1_t *layer_shell_surface, uint32_t *panel_width, uint32_t *panel_height)
{
  m_compositor = compositor;
  m_display =  display;
  m_xdg_wm_base = xdg_wm_base;
  m_shm = shm;
  m_layer_shell_surface = layer_shell_surface;
  m_panel_width = panel_width;
  m_panel_height = panel_height;

  m_shared_mem = nullptr;
  m_cairo_surface = nullptr;

  m_width = m_height = 32;

  add_item(std::make_shared<Button>("edit-undo", "Hola mundo"));
  add_item(std::make_shared<Button>("firefox", "Firefox"));
}


Popup::~Popup()
{
  hide();
}



void Popup::hide()
{
  if(m_shared_mem) {
    m_xdg_popup.proxy_release();
    m_xdg_positioner.proxy_release();
    m_buffer.at(0).proxy_release();
    m_buffer.at(1).proxy_release();
    m_xdg_surface.proxy_release();
    m_surface.proxy_release();
    m_shared_mem = nullptr;
    cairo_surface_destroy(m_cairo_surface);
    m_cairo_surface = nullptr;
  }
}

void Popup::show(int offset)
{
  debug << "Popup offset: " << offset << std::endl;
  if(offset < 1) offset = 1; // xdg_positioner fails if offset is 0

  // Close previous tooltip
  hide();

  // Reset tooltip size
  m_width = m_height = 32;

  create_wayland_surface(offset);

  //find_size_for_text(std::string("Hola mundo"));

  cairo_t *cr = cairo_create(m_cairo_surface);
  paint(cr);
  cairo_destroy(cr);
  debug << "Popup m_width: " << m_width << std::endl;
  printf("\nPopup size: %d, %d\n\n", m_width, m_height);

  // Close tool build for compute text size
  hide();
  debug << "Popup m_width: " << m_width << std::endl;
  printf("\nPopup size: %d, %d\n\n", m_width, m_height);

  // Build a new window with the right size
  if(m_width < 1) m_width = 1;
  if(m_height < 1) m_height = 1;
  create_wayland_surface(offset);
  //draw_text(std::string("Hola mundo"));
  cr = cairo_create(m_cairo_surface);
  paint(cr);
  cairo_destroy(cr);
}

void Popup::create_wayland_surface(int offset)
{
  m_surface = m_compositor->create_surface();
  m_xdg_surface = m_xdg_wm_base->get_xdg_surface(m_surface);
  m_xdg_surface.on_configure() = [&] (uint32_t serial) { 
    if(!m_shared_mem) {
      m_shared_mem = std::make_shared<shared_mem_t>(2*m_width*m_height*4);
      auto pool = m_shm->create_pool(m_shared_mem->get_fd(), 2*m_width*m_height*4);
      for(unsigned int c = 0; c < 2; c++)
        m_buffer.at(c) = pool.create_buffer(c*m_width*m_height*4, m_width, m_height, m_width*4, shm_format::argb8888);
    }
    m_xdg_surface.set_window_geometry(0, 0, m_width, m_height);
    m_xdg_surface.ack_configure(serial); 
  };
  m_xdg_positioner = m_xdg_wm_base->create_positioner();
  {
    debug << "m_xdg_positioner set_size " << m_width << ", " << m_height << std::endl;
    int width = m_width <= 0 ? 32 : m_width;
    int height = m_height <= 0 ? 32 : m_height;
    m_xdg_positioner.set_size(width, height);
  }
  debug << "m_xdg_positioner offset " << offset << std::endl;
  switch(Settings::get_settings()->panel_position()) {
    case PanelPosition::BOTTOM:
      m_xdg_positioner.set_anchor_rect(0, 0, offset , *m_panel_height);
      m_xdg_positioner.set_gravity(xdg_positioner_gravity::top);
      m_xdg_positioner.set_constraint_adjustment(xdg_positioner_constraint_adjustment::slide_x);
      m_xdg_positioner.set_anchor(xdg_positioner_anchor::top_right);
      break;
    case PanelPosition::TOP:
      m_xdg_positioner.set_anchor_rect(0, 0, offset , *m_panel_height);
      m_xdg_positioner.set_gravity(xdg_positioner_gravity::bottom);
      m_xdg_positioner.set_constraint_adjustment(xdg_positioner_constraint_adjustment::slide_x);
      m_xdg_positioner.set_anchor(xdg_positioner_anchor::bottom_right);
      break;
  }
  m_xdg_popup = m_xdg_surface.get_popup(nullptr, m_xdg_positioner);
  m_xdg_popup.on_configure() = [&] (int32_t x, int32_t y, int32_t w, int32_t h) {
    //draw_tooltip();
  };
  m_xdg_popup.on_popup_done() = [&] () {
    //std::cout << "Popup done\n";
  };
  m_layer_shell_surface->get_popup(m_xdg_popup);

  m_surface.commit();
  m_display->roundtrip();

  // New cairo surface
  if(!m_surface || !m_shared_mem)
    return;

  if(m_cairo_surface == nullptr) {
    m_cairo_surface = cairo_image_surface_create_for_data(
        (unsigned char*)(m_shared_mem->get_mem()), 
        CAIRO_FORMAT_ARGB32, m_width, m_height, 
        cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, m_width)
    );

    if(cairo_surface_status(m_cairo_surface) == CAIRO_STATUS_SUCCESS) {
      debug << "New cairo_surface\n";
    } else {
      debug_error << "cairo_surface cannot be created: " << cairo_status_to_string(cairo_surface_status(m_cairo_surface)) << std::endl;
    }
  }
}


void Popup::find_size_for_text(const std::string & text)
{
  cairo_t *cr = cairo_create(m_cairo_surface);
  
  // Get lines of text
  std::vector<std::string> lines = get_lines(text);

  cairo_select_font_face(cr, Settings::get_settings()->font().c_str(), CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size(cr, Settings::get_settings()->font_size());
  m_width = 0; 
  m_height = popup_margin/2;
  for(std::string line : lines) {
    cairo_text_extents_t extents;
    cairo_text_extents(cr, line.c_str(), &extents);
    if(m_width < extents.width)
      m_width = extents.width + popup_margin;
    m_height += extents.height + popup_margin/2.0;
  }
  
  cairo_destroy(cr);
}


void Popup::draw_text(const std::string & text)
{
  std::vector<std::string> lines = get_lines(text);

  cairo_t *cr = cairo_create(m_cairo_surface);

  Color color = Settings::get_settings()->color();
  Color background_color = Settings::get_settings()->background_color();
  cairo_set_source_rgba (cr, background_color.red, background_color.green, background_color.blue, 0.5);
  cairo_rectangle (cr, 0, 0, m_width, m_height);
  cairo_fill(cr);
  cairo_set_source_rgba (cr, color.red, color.green, color.blue, 0.5);
  cairo_rectangle (cr, 0, 0, m_width, m_height);
  cairo_stroke(cr);

  cairo_set_source_rgba (cr, color.red, color.green, color.blue, 1.0);
  cairo_save(cr);
  cairo_rectangle(cr, 0, 0, m_width, m_height);
  cairo_clip(cr);
  cairo_set_source_rgba (cr, color.red, color.green, color.blue, 1.0);
  cairo_select_font_face(cr, Settings::get_settings()->font().c_str(), CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size(cr, Settings::get_settings()->font_size());

  const int sep = popup_margin/2;
  int y = sep;
  for(std::string line : lines) {
    cairo_text_extents_t extents;
    cairo_text_extents(cr, line.c_str(), &extents);
    cairo_move_to(cr, popup_margin/2.0 + (m_width - popup_margin - extents.width)/2, y + extents.height);
    cairo_show_text(cr, line.c_str());
    y += extents.height + sep;
  }
  cairo_restore(cr);

  cairo_destroy(cr);

  m_surface.attach(m_buffer.at(0), 0, 0);
  m_surface.damage(0, 0, m_width, m_height);
  m_surface.commit();
}

void Popup::add_item(std::shared_ptr<PanelItem> item)
{
  m_items.push_back(item);
}

void Popup::paint(cairo_t *cr, bool update_items_only)
{
  if(! update_items_only) {
    Color background_color = Settings::get_settings()->background_color();
    cairo_set_source_rgba (cr, background_color.red, background_color.green, background_color.blue, 0.5);
    cairo_rectangle (cr, 0, 0, m_width, m_height);
    cairo_fill(cr);
  }

  const uint32_t sep = popup_margin/2;
  uint32_t y = sep;
  for(auto item : m_items) {
    if(!update_items_only || update_items_only && item->need_repaint()) {
      if(! update_items_only) {
        item->update_size(cr);
        item->set_pos(sep, y);
        item->set_height(Settings::get_settings()->font_size());
      }
      item->repaint(cr);
      if(update_items_only) {
        m_surface.damage(item->get_x(), item->get_y(), item->get_width(), item->get_height());
      }
      if(! update_items_only) {
        y += item->get_height();
        m_height = y;
        if(m_width < (item->get_width() + popup_margin + item->get_height())) 
          m_width = item->get_width() + popup_margin + item->get_height();
      }
    }
  }

  if(! update_items_only) {
    m_height += sep;
    Color color = Settings::get_settings()->color();
    cairo_set_source_rgba (cr, color.red, color.green, color.blue, 0.5);
    cairo_rectangle (cr, 0, 0, m_width, m_height);
    cairo_stroke(cr);
    m_surface.attach(m_buffer.at(0), 0, 0);
    m_surface.damage(0, 0, m_width, m_height);
  } else {
    m_surface.attach(m_buffer.at(0), 0, 0);
  }

  m_surface.commit();
}

void Popup::on_mouse_enter(uint32_t x, uint32_t y)
{
  for(auto item : m_items) {
    item->on_mouse_enter(x, y);
  }
  cairo_t *cr = cairo_create(m_cairo_surface);
  paint(cr, true);
  cairo_destroy(cr);
}

void Popup::on_mouse_leave(int x, int y, bool leave)
{
  for(auto item : m_items) {
    item->on_mouse_leave(x, y, leave);
  }
  cairo_t *cr = cairo_create(m_cairo_surface);
  paint(cr, true);
  cairo_destroy(cr);
}

void Popup::on_mouse_clicked(int x, int y, int button)
{
  for(auto item : m_items) {
    item->on_mouse_clicked(x, y, button);
  }
}

void Popup::on_mouse_released(int x, int y)
{
  for(auto item : m_items) {
    item->on_mouse_released(x, y);
  }
}
