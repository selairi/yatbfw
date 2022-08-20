
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
 
#ifndef _PANEL_H_
#define _PANEL_H_

#include <wayland-client.hpp>
#include <wayland-client-protocol-extra.hpp>
#include <linux/input.h>
#include <wayland-cursor.hpp>
#include <cairo/cairo.h>

#include <layer-shell.h>
#include <toplevel.h>
#include "button.h"
#include "toplevelbutton.h"
#include "tooltip.h"

#include <memory>

using namespace wayland;

class shared_mem_t;

/** \class Panel
 *  \brief Main class that draws a panel.
 *
 *  This class draws a panel an controls wayland connection,
 *  event loop and paints items in panel.
 */
class Panel
{
public:
  Panel(const Panel&) = delete;
  Panel(Panel&&) noexcept = delete;
  ~Panel() noexcept = default;
  Panel& operator=(const Panel&) = delete;
  Panel& operator=(Panel&&) noexcept = delete;
  Panel();

  void init();
  void run();

  void add_launcher(const std::string & icon, const std::string & text, const std::string & tooltip, const std::string & exec, bool start_pos = true);
  void add_clock(const std::string & icon, const std::string & format, const std::string & exec, bool start_pos = true);
  void add_battery(
     const std::string & icon_battery_full,    
     const std::string & icon_battery_good,    
     const std::string & icon_battery_medium,  
     const std::string & icon_battery_low,     
     const std::string & icon_battery_empty,   
     const std::string & icon_battery_charging,
     const std::string & icon_battery_charged, 
     bool no_text,
     const std::string & exec, bool start_pos = true);
  void show_tooltip();


private:
  void draw(uint32_t serial = 0, bool update_items_only = false);
  void on_toplevel_listener(zwlr_foreign_toplevel_handle_v1_t handle);

  // global objects
  display_t display;
  registry_t registry;
  compositor_t compositor;
  //shell_t shell;
  xdg_wm_base_t xdg_wm_base;
  seat_t seat;
  shm_t shm;

  // local objects
  surface_t surface;
  shell_surface_t shell_surface;
  xdg_surface_t xdg_surface;
  xdg_toplevel_t xdg_toplevel;
  pointer_t pointer;
  keyboard_t keyboard;
  callback_t frame_cb;
  cursor_image_t cursor_image;
  buffer_t cursor_buffer;
  surface_t cursor_surface;
  zwlr_layer_shell_v1_t layer_shell;
  zwlr_layer_surface_v1_t layer_shell_surface;
  zwlr_foreign_toplevel_manager_v1_t toplevel_manager;
  std::vector<std::shared_ptr<ToplevelButton> > m_toplevel_handles;
  output_t output;
  cairo_surface_t *cairo_surface;

  // Tooltip objects
  surface_t tooltip_surface;
  xdg_surface_t tooltip_xdg_surface;
  xdg_positioner_t tooltip_xdg_positioner;
  xdg_popup_t tooltip_xdg_popup;
  std::shared_ptr<shared_mem_t> tooltip_shared_mem;
  std::array<buffer_t, 2> tooltip_buffer;
  cairo_surface_t *tooltip_cairo_surface;
  void draw_tooltip(int width, int height);

  uint32_t m_width, m_height;
  std::vector<std::shared_ptr<PanelItem> > m_panel_items;
  uint32_t m_last_cursor_x, m_last_cursor_y;
  uint32_t m_toplevel_items_offset;

  std::shared_ptr<shared_mem_t> shared_mem;
  std::array<buffer_t, 2> buffer;
  int cur_buf;
  
  ToolTip tooltip;

  bool running;
  bool has_pointer;
  bool has_keyboard;
  bool m_repaint_full;
  bool m_repaint_partial;
};

#endif
