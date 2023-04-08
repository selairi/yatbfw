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
 
#ifndef __POPUP_H__
#define __POPUP_H__

#include <wayland-client.hpp>
#include <wayland-client-protocol-extra.hpp>
#include <linux/input.h>
#include <wayland-cursor.hpp>
#include <layer-shell.h>
#include <cairo/cairo.h>
#include <string>
#include <memory>
#include "panelitem.h"

using namespace wayland;

class shared_mem_t;

/*! \class Popup
 *  \brief Shows a popup window.
 *
 *  Shows a floating window.
 */
class Popup
{
public:
  virtual ~Popup();

  Popup(compositor_t *compositor, display_t *display, xdg_wm_base_t *xdg_wm_base, shm_t *shm, zwlr_layer_surface_v1_t *layer_shell_surface, uint32_t *panel_width, uint32_t *panel_height);
  
  void show(int offset);
  void hide();
  bool is_visible();

  void add_item(std::shared_ptr<PanelItem> item);

  inline const surface_t get_surface() {return m_surface;};
  
  void paint(cairo_t *cr, bool update_items_only = false);

  void on_mouse_enter(int x, int y);
  void on_mouse_leave(int x, int y, bool leave);
  void on_mouse_clicked(int x, int y, int button);
  void on_mouse_released(int x, int y);
private:
  void draw_text(const std::string & text);
  void find_size_for_text(const std::string & text);
  void create_wayland_surface(int offset);


  // Objects shared with Panel
  compositor_t *m_compositor;
  display_t *m_display;
  xdg_wm_base_t *m_xdg_wm_base;
  shm_t *m_shm;
  zwlr_layer_surface_v1_t *m_layer_shell_surface;
  uint32_t *m_panel_width, *m_panel_height;

  // Popup objects
  surface_t m_surface;
  xdg_surface_t m_xdg_surface;
  xdg_positioner_t m_xdg_positioner;
  xdg_popup_t m_xdg_popup;
  std::shared_ptr<shared_mem_t> m_shared_mem;
  std::array<buffer_t, 2> m_buffer;
  cairo_surface_t *m_cairo_surface;
  uint32_t m_width, m_height;
  bool m_visible;

  // Items in popup
  std::vector<std::shared_ptr<PanelItem> > m_items;
};

#endif
