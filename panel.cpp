
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
#include <stdexcept>
#include <iostream>
#include <array>
#include <memory>
#include <sstream>
#include <ctime>
#include <algorithm>
#include <random>

#include <wayland-client.hpp>
#include <wayland-client-protocol-extra.hpp>
#include <linux/input.h>
#include <wayland-cursor.hpp>

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <time.h>

#include <cairo/cairo.h>

#include "layer-shell.h"
#include "toplevel.h"
#include "buttonruncommand.h"
#include "clock.h"
#include "battery.h"
#include "panel.h"
#include "settings.h"

#define WIDTH 34
#define HEIGHT 34

using namespace wayland;

#include "shared_mem.hpp"

void Panel::draw(uint32_t serial, bool update_items_only)
{
  if(!surface || !shared_mem)
    return;

  if(cairo_surface == nullptr) {
    cairo_surface = cairo_image_surface_create_for_data((unsigned char*)(shared_mem->get_mem()), CAIRO_FORMAT_ARGB32, m_width, m_height, /*stride*/ m_width*4);

    if(cairo_surface_status(cairo_surface) == CAIRO_STATUS_SUCCESS) {
      debug << "New cairo_surface\n";
    } else {
      debug_error << "cairo_surface cannot be created: " 
        << cairo_status_to_string(cairo_surface_status(cairo_surface)) 
        << std::endl;
    }
  }

  Color color = Settings::get_settings()->color();
  Color background_color = Settings::get_settings()->background_color();

  cairo_t *cr = cairo_create(cairo_surface);
  cairo_set_source_rgba (cr, color.red, color.green, color.blue, 0);
  cairo_paint(cr);

  // Draw window frame
  if(! update_items_only) {
    cairo_set_source_rgba (cr, background_color.red, background_color.green, background_color.blue, 1.0);
    cairo_rectangle (cr, 0, 0, m_width, m_height);
    cairo_fill(cr);
  }

  // Draw panel items
  uint32_t x_start = 0, x_end = m_width;
  for(auto item : m_panel_items) {
    uint32_t x = item->is_start_pos() ? x_start : (x_end - item->get_width());
    item->set_pos(x, 0);
    if(update_items_only) {
      if(item->need_repaint()) {
        item->repaint(cr);
        surface.damage(item->get_x(), item->get_y(), item->get_width(), item->get_height());
      }
    } else {
      item->repaint(cr);
    }
    if(item->is_start_pos())
      x_start += item->get_width();
    else
      x_end = x;
  }

  // Toplevel items are drawn centered
  cairo_save(cr);
  cairo_rectangle(cr, x_start, 0, x_end - x_start, m_height);
  cairo_clip(cr);
  uint32_t x_toplevels = 0, n_toplevels = 0;
  for(std::shared_ptr<ToplevelButton> item : m_toplevel_handles) {
    x_toplevels += item->get_width();
    n_toplevels++;
  }
  // Change toplevel items offset if there is not enoght space 
  // and user moves the mouse wheel
  if(x_toplevels > (x_end - x_start))
    x_toplevels += m_toplevel_items_offset;
  x_toplevels = (x_start + x_end - x_toplevels) / 2;

  for(std::shared_ptr<ToplevelButton> item : m_toplevel_handles) {
    item->set_pos(x_toplevels, 0);
    if(update_items_only) {
      if(item->need_repaint()) {
        item->repaint(cr);
        surface.damage(item->get_x(), item->get_y(), item->get_width(), item->get_height());
      }
    } else {
      item->repaint(cr);
    }
    x_toplevels += item->get_width(); 
  }
  cairo_restore(cr);


  cairo_destroy(cr);

  surface.attach(buffer.at(0), 0, 0);
  if(! update_items_only)
    surface.damage(0, 0, m_width, m_height);

  surface.commit();
  debug << "draw finished\n";
}

Panel::Panel()
{
  m_width = Settings::get_settings()->panel_size();
  m_height = Settings::get_settings()->panel_size();
  m_last_cursor_x = m_last_cursor_y = 0;
  m_toplevel_items_offset = 0;
  cairo_surface = nullptr;
  m_repaint_full = true;
  m_repaint_partial = false;
  tooltip_cairo_surface = nullptr;
  tooltip_shared_mem = nullptr;
}

void Panel::init()
{
  m_width = m_height = Settings::get_settings()->panel_size();
  // retrieve global objects
  registry = display.get_registry();
  registry.on_global() = [&] (uint32_t name, const std::string& interface, uint32_t version)
  {
    debug << "Found interface " << interface << " version " << version << std::endl;

    if(interface == compositor_t::interface_name) {
      registry.bind(name, compositor, version);
      debug << "Binding interface " << compositor_t::interface_name << std::endl;
    } /*else if(interface == shell_t::interface_name) {
        std::cout << "registrando " << shell_t::interface_name << std::endl;
        registry.bind(name, shell, version);
        }*/ 
    else if(interface == xdg_wm_base_t::interface_name) {
      debug << "Binding interface " << xdg_wm_base_t::interface_name << std::endl;
      registry.bind(name, xdg_wm_base, version);
    } else if(interface == seat_t::interface_name) {
      debug << "Binding interface " << seat_t::interface_name << std::endl;
      registry.bind(name, seat, version);
    } else if(interface == shm_t::interface_name) {
      debug << "Binding interface " << shm_t::interface_name << std::endl;
      registry.bind(name, shm, version);
    } else if(interface == zwlr_layer_shell_v1_t::interface_name) {
      debug << "Binding interface " << zwlr_layer_shell_v1_t::interface_name << std::endl;
      registry.bind(name, layer_shell, version);
    } else if(interface == zwlr_foreign_toplevel_manager_v1_t::interface_name) {
      debug << "Binding interface " << zwlr_foreign_toplevel_manager_v1_t::interface_name << std::endl;
      registry.bind(name, toplevel_manager, version);
      toplevel_manager.on_toplevel() = [&](zwlr_foreign_toplevel_handle_v1_t handle) {
        debug << "  toplevel_manager::on_toplevel" << std::endl;
        on_toplevel_listener(handle);
      };
    } else if(interface == output_t::interface_name) {
      debug << "Binding interface " << output_t::interface_name << std::endl;
      registry.bind(name, output, version);
      debug << "Binding interface finished." << std::endl;
      output.on_mode() = [&](uint32_t flags, int32_t width, int32_t height, int32_t refresh) {
        m_width = width;
      };
    }

  };
  debug << "First roundtrip has been started" << std::endl;
  display.roundtrip();
  debug << "First roundtrip has been finished" << std::endl;

  // Check if all interfaces has been loaded
  if(!compositor) 
    throw debug_get_func + "wl_compositor interface cannot been loaded from Wayland compositor.";
  else if(!seat) 
    throw debug_get_func + "wl_seat interface cannot been loaded from Wayland compositor.";
  else if(!shm) 
    throw debug_get_func + "wl_shm interface cannot been loaded from Wayland compositor.";
  else if(!layer_shell) 
    throw debug_get_func + "layer_shell interface cannot been loaded from Wayland compositor.";
  else if(!toplevel_manager) 
    throw debug_get_func + "foreign_toplevel interface cannot been loaded from Wayland compositor.";
  else if(!output) 
    throw debug_get_func + "wl_output interface cannot been loaded from Wayland compositor.";

  seat.on_capabilities() = [&] (const seat_capability& capability)
  {
    has_keyboard = capability & seat_capability::keyboard;
    has_pointer = capability & seat_capability::pointer;
  };

  // Load outputs sizes
  display.roundtrip();

  // create a surface
  surface = compositor.create_surface();

  // create a shell surface
  if(layer_shell) {
    layer_shell_surface = layer_shell.get_layer_surface(surface, output, zwlr_layer_shell_v1_layer::top, std::string("Window"));
    switch(Settings::get_settings()->panel_position()) {
      case PanelPosition::TOP:
        layer_shell_surface.set_anchor(zwlr_layer_surface_v1_anchor::top);
        break;
      default:
        layer_shell_surface.set_anchor(zwlr_layer_surface_v1_anchor::bottom);
    }
    layer_shell_surface.set_size(m_width, m_height);
    layer_shell_surface.set_exclusive_zone(Settings::get_settings()->exclusive_zone());
    layer_shell_surface.set_keyboard_interactivity(zwlr_layer_surface_v1_keyboard_interactivity::none);
    layer_shell_surface.on_configure() = [&](uint32_t serial, uint32_t width, uint32_t height) {
      if(m_width != width || m_height != height) {
        m_width = width;
        m_height = height; 
        debug << "[layer_shell_surface.on_configure()] " << width << " x " << height << std::endl;
        layer_shell_surface.set_size(m_width, m_height);
        layer_shell_surface.set_exclusive_zone(Settings::get_settings()->exclusive_zone());
        surface.damage(0, 0, m_width, m_height);
        surface.commit();
      }
      layer_shell_surface.ack_configure(serial);
    };
  } else if(xdg_wm_base) {
    xdg_wm_base.on_ping() = [&] (uint32_t serial) { xdg_wm_base.pong(serial); };
    xdg_surface = xdg_wm_base.get_xdg_surface(surface);
    xdg_surface.on_configure() = [&] (uint32_t serial) { xdg_surface.ack_configure(serial); };
    xdg_toplevel = xdg_surface.get_toplevel();
    xdg_toplevel.set_title("Window");
    xdg_toplevel.on_close() = [&] () { running = false; };
  }

  if(xdg_wm_base) {
    xdg_wm_base.on_ping() = [&] (uint32_t serial) { xdg_wm_base.pong(serial); };
  }

  surface.commit();

  debug << "Second roundtrip has been started" << std::endl;
  display.roundtrip();
  debug << "Second roundtrip has been finished" << std::endl;

  // Get input devices
  if(!has_keyboard)
    throw std::runtime_error(debug_get_func + "No keyboard found.");
  if(!has_pointer)
    throw std::runtime_error(debug_get_func + "No pointer found.");

  pointer = seat.get_pointer();
  keyboard = seat.get_keyboard();

  // create shared memory
  shared_mem = std::make_shared<shared_mem_t>(2*m_width*m_height*4);
  auto pool = shm.create_pool(shared_mem->get_fd(), 2*m_width*m_height*4);
  for(unsigned int c = 0; c < 2; c++)
    buffer.at(c) = pool.create_buffer(c*m_width*m_height*4, m_width, m_height, m_width*4, shm_format::argb8888);
  cur_buf = 0;

  // load cursor theme
  debug << "Cursor theme " << Settings::get_settings()->cursor_theme() << std::endl;
  std::string theme(Settings::get_settings()->cursor_theme());
  debug << "Cursor size " << Settings::get_settings()->cursor_size() << std::endl;
  cursor_theme_t cursor_theme = cursor_theme_t(theme, Settings::get_settings()->cursor_size(), shm);
  debug << "Cursor theme loaded" << std::endl; 
  cursor_t cursor = cursor_theme.get_cursor("left_ptr");
  debug << "Cursor arrow loaded" << std::endl;
  cursor_image = cursor.image(0);
  cursor_buffer = cursor_image.get_buffer();
  debug << "Ready cursor theme" << std::endl;

  // create cursor surface
  cursor_surface = compositor.create_surface();

  // draw cursor
  pointer.on_enter() = [&] (uint32_t serial, const surface_t& /*unused*/, int32_t x, int32_t y)
  {
    cursor_surface.attach(cursor_buffer, 0, 0);
    cursor_surface.damage(0, 0, cursor_image.width(), cursor_image.height());
    cursor_surface.commit();
    pointer.set_cursor(serial, cursor_surface, 0, 0);
    debug << "Cursor " << x << y << std::endl;
    m_last_cursor_x = x;
    m_last_cursor_y = y;
    for(auto item : m_panel_items)
      item->on_mouse_enter(x, y);
    for(std::shared_ptr<ToplevelButton> item : m_toplevel_handles)
      item->on_mouse_enter(x, y);

    m_repaint_partial = true;
    //draw(serial, true);
  };

  pointer.on_leave() = [&] (uint32_t serial, const surface_t& /*unused*/)
  {
    debug << "on_leave\n";
    for(auto item : m_panel_items)
      item->on_mouse_leave(m_last_cursor_x, m_last_cursor_y, true);
    for(std::shared_ptr<ToplevelButton> item : m_toplevel_handles)
      item->on_mouse_leave(m_last_cursor_x, m_last_cursor_y, true);
    m_repaint_partial = true;
    ToolTip::hide();
  };

  pointer.on_motion() = [&] (uint32_t time, double x, double y)
  {
    //printf("Cursor %f,%f\n", x, y);
    m_last_cursor_x = x;
    m_last_cursor_y = y;
    for(auto item : m_panel_items) {
      item->on_mouse_enter(x, y);
      item->on_mouse_leave(x, y, false);
    }
    for(std::shared_ptr<ToplevelButton> item : m_toplevel_handles) {
      item->on_mouse_enter(x, y);
      item->on_mouse_leave(x, y, false);
    }
    m_repaint_partial = true;
    //draw(-1, true);
  };

  pointer.on_button() = [&] (uint32_t serial, uint32_t /*unused*/, uint32_t button, pointer_button_state state)
  {
    debug << "Button action  " << button << std::endl;
    if(/*(button == BTN_LEFT || button == BTN_RIGHT) && */state == pointer_button_state::pressed) {
      debug << "Button pressed\n";
      //showToolTip();

      for(auto item : m_panel_items)
        item->on_mouse_clicked(m_last_cursor_x, m_last_cursor_y, button);
      for(std::shared_ptr<ToplevelButton> item : m_toplevel_handles)
        //((PanelItem*)item)->on_mouse_clicked(m_last_cursor_x, m_last_cursor_y, button);
        item->on_mouse_clicked(m_last_cursor_x, m_last_cursor_y, button);
      m_repaint_partial = true;
      //draw(serial, true);
    } else if(/*(button == BTN_LEFT || button == BTN_RIGHT) && */state != pointer_button_state::pressed) {
      for(auto item : m_panel_items)
        item->on_mouse_released(m_last_cursor_x, m_last_cursor_y);
      for(std::shared_ptr<ToplevelButton> item : m_toplevel_handles)
        item->on_mouse_released(m_last_cursor_x, m_last_cursor_y);
      m_repaint_partial = true;
      //draw(serial, true);
    }
  };

  pointer.on_axis() = [&] (uint32_t time, pointer_axis axis, double value) {
    // Change toplevel items offset if there is not enoght space 
    // and user moves the mouse wheel
    //if(axis == pointer_axis::vertical_scroll)
    //  printf("on_axis: axis vertical\n");
    //else
    //  printf("on_axis: axis horizontal\n");
    m_toplevel_items_offset += (value > 0.0 ? 1 : -1) * Settings::get_settings()->panel_size();
    m_repaint_full = true;
    //draw();
  };

  // press 'q' to exit
  keyboard.on_key() = [&] (uint32_t /*unused*/, uint32_t /*unused*/, uint32_t key, keyboard_key_state state)
  {
    if(key == KEY_Q && state == keyboard_key_state::pressed)
      running = false;
  };

  // draw stuff
  debug << "Ready to draw" << std::endl;
  draw();
  draw();
  debug << "Drawn" << std::endl;

  tooltip.init(&compositor, &display, &xdg_wm_base, &shm, &layer_shell_surface, &m_width, &m_height);
}


void Panel::on_toplevel_listener(zwlr_foreign_toplevel_handle_v1_t toplevel_handle)
{
  auto toplevel = std::make_shared<ToplevelButton>(toplevel_handle, seat, &m_toplevel_handles);
  toplevel->set_width(Settings::get_settings()->panel_size());
  toplevel->set_height(Settings::get_settings()->panel_size());
  toplevel->repaint_main_interface = [&](bool update_items_only) {
    if(update_items_only)
      m_repaint_partial = true;
    else
      m_repaint_full = true;
  };
  if(! toplevel)
    debug_error << "No free memory" << std::endl;
  else
    m_toplevel_handles.push_back(toplevel);
  m_repaint_full = true;
}


void Panel::add_launcher(const std::string & icon, const std::string & text, const std::string & tooltip, const std::string & exec, bool start_pos)
{
  auto n = std::make_shared<ButtonRunCommand>(icon, text, tooltip);
  n->set_command(exec);
  n->set_fd(display.get_fd());
  n->set_width(Settings::get_settings()->panel_size() - 1);
  n->set_height(Settings::get_settings()->panel_size() - 1);
  n->set_start_pos(start_pos);
  m_panel_items.push_back(n);
}

void Panel::add_clock(const std::string & icon, const std::string & format, const std::string & exec, bool start_pos)
{
  auto c = std::make_shared<Clock>(icon, format);
  c->set_width(Settings::get_settings()->panel_size() - 1);
  c->set_height(Settings::get_settings()->panel_size() - 1);
  c->set_command(exec);
  c->send_repaint = [&]() {
    //draw(-1, true);
    m_repaint_partial = true;
  };
  c->set_fd(display.get_fd());
  c->set_start_pos(start_pos);
  m_panel_items.push_back(c);
}

void Panel::add_battery(
   const std::string & icon_battery_full,    
   const std::string & icon_battery_good,    
   const std::string & icon_battery_medium,  
   const std::string & icon_battery_low,     
   const std::string & icon_battery_empty,   
   const std::string & icon_battery_charging,
   const std::string & icon_battery_charged,
   bool no_text,
   const std::string & exec, bool start_pos
)
{
  auto c = std::make_shared<Battery>(
      icon_battery_full,    
      icon_battery_good,    
      icon_battery_medium,  
      icon_battery_low,     
      icon_battery_empty,   
      icon_battery_charging,
      icon_battery_charged,
      no_text
  );
  c->set_width(Settings::get_settings()->panel_size() - 1);
  c->set_height(Settings::get_settings()->panel_size() - 1);
  c->set_command(exec);
  c->send_repaint = [&]() {
    //draw(-1, true);
    m_repaint_partial = true;
  };
  c->set_fd(display.get_fd());
  c->set_start_pos(start_pos);
  m_panel_items.push_back(c);
}

static long get_time_milliseconds()
{
  struct timespec time_aux;
  clock_gettime(CLOCK_REALTIME, &time_aux);
  return time_aux.tv_sec * 1000 + time_aux.tv_nsec / 1000000;
}

void Panel::run()
{
  // Main event loop
  // This loop stops when runnig is false
  // Sends time out each second
  running = true;
  struct pollfd fds[1];
  int timeout_msecs = -1;
  int ret;
  long now_in_msecs;

  fds[0].fd = display.get_fd();
  fds[0].events = POLLIN;
  fds[0].revents = 0;

  while(running) {
    // Update timeout and run timeout events
    now_in_msecs = get_time_milliseconds();
    timeout_msecs = -1;
    for(auto item : m_panel_items) {
      long item_timeout = item->next_time_timeout(now_in_msecs);
      if(item_timeout >= 0) {
        if(now_in_msecs >= item_timeout) {
          item->on_timeout(now_in_msecs);
          item_timeout = item->next_time_timeout(now_in_msecs);
        }
        long timeout = item_timeout - now_in_msecs;
        if(timeout_msecs < 0 || timeout < timeout_msecs)
          timeout_msecs = timeout;
      }
    }
    debug << "Timeout " << timeout_msecs << std::endl;
    // Repaint interface
    if(m_repaint_full)
      draw();
    else if(m_repaint_partial)
      draw(-1, true);
    // Proccess pending Wayland events
    display.dispatch_pending();
    display.flush();
    m_repaint_full = m_repaint_partial = false;
    // Wait for events
    ret = poll(fds, sizeof(fds) / sizeof(fds[0]), timeout_msecs);
    if(ret > 0) {
      if(fds[0].revents) {
        display.dispatch();
        fds[0].revents = 0;
      }
    } else if(ret == 0) {
      debug << "Timeout\n";
      //for(auto item : m_panel_items) {
      //  item->on_timeout();
      //}
    } else {
      debug << "poll failed %d" << ret << std::endl;
    }
  }
}


void Panel::draw_tooltip(int width, int height)
{
  if(!tooltip_surface || !tooltip_shared_mem)
    return;

  if(tooltip_cairo_surface == nullptr) {
    tooltip_cairo_surface = cairo_image_surface_create_for_data((unsigned char*)(tooltip_shared_mem->get_mem()), CAIRO_FORMAT_ARGB32, width, height, /*stride*/ width*4);

    if(cairo_surface_status(tooltip_cairo_surface) == CAIRO_STATUS_SUCCESS) {
      debug << "New cairo_surface\n";
    } else {
      debug_error << "cairo_surface cannot be created: " << cairo_status_to_string(cairo_surface_status(tooltip_cairo_surface)) << std::endl;
    }
  }

  Color color = Settings::get_settings()->color();
  Color background_color = Settings::get_settings()->background_color();

  cairo_t *cr = cairo_create(tooltip_cairo_surface);
  cairo_set_source_rgba (cr, color.red, color.green, color.blue, 0);
  cairo_paint(cr);

  cairo_set_source_rgba (cr, color.red, color.green, color.blue, 1.0);
  cairo_rectangle (cr, 0, 0, width, height);
  cairo_fill(cr);
  cairo_paint(cr);

  cairo_destroy(cr);

  tooltip_surface.attach(tooltip_buffer.at(0), 0, 0);
  tooltip_surface.damage(0, 0, width, height);

  tooltip_surface.commit();
  debug << "draw finished\n";
}

void Panel::show_tooltip()
{
  int width = 50, height = 32;
  if(tooltip_cairo_surface) {
    tooltip_xdg_popup.proxy_release();
    tooltip_xdg_positioner.proxy_release();
    std::cout << "Eliminando buffer\n";
    tooltip_buffer.at(0).proxy_release();
    tooltip_buffer.at(1).proxy_release();
    tooltip_xdg_surface.proxy_release();
    tooltip_surface.proxy_release();
    tooltip_shared_mem = nullptr;

    debug_error << "Show tooltip" << std::endl << std::endl;;
    //draw_tooltip(width, height);
    return;
  }

  tooltip_surface = compositor.create_surface();
  tooltip_xdg_surface = xdg_wm_base.get_xdg_surface(tooltip_surface);
  tooltip_xdg_surface.on_configure() = [&] (uint32_t serial) { 
    debug_error << "Tooltip" << std::endl;
    int width = 50, height = 32;
    if(!tooltip_shared_mem) {
      tooltip_shared_mem = std::make_shared<shared_mem_t>(2*width*height*4);
      auto pool = shm.create_pool(tooltip_shared_mem->get_fd(), 2*width*height*4);
      for(unsigned int c = 0; c < 2; c++)
        tooltip_buffer.at(c) = pool.create_buffer(c*width*height*4, width, height, width*4, shm_format::argb8888);
    }
    tooltip_xdg_surface.set_window_geometry(1920*0, 1260*0, width, height);
    tooltip_xdg_surface.ack_configure(serial); 
  };
  tooltip_xdg_positioner = xdg_wm_base.create_positioner();
  tooltip_xdg_positioner.set_size(width, height);
  tooltip_xdg_positioner.set_anchor_rect(1920/2*0, 1260*0, m_width, m_height);
  tooltip_xdg_positioner.set_anchor(xdg_positioner_anchor::top);
  tooltip_xdg_popup = tooltip_xdg_surface.get_popup(nullptr, tooltip_xdg_positioner);
  tooltip_xdg_popup.on_configure() = [&] (int32_t x, int32_t y, int32_t w, int32_t h) {
    debug_error << "popup: x:" << x << " y:" << y << " w:" << w << " h:" << h << std::endl;
    //tooltip_xdg_surface.set_window_geometry(x, y, w, h);
    draw_tooltip(w, h);
  };
  tooltip_xdg_popup.on_popup_done() = [&] () {
    std::cout << "Popup done\n";
  };
  layer_shell_surface.get_popup(tooltip_xdg_popup);

  tooltip_surface.commit();
  display.roundtrip();
  std::cout << "Draw tooltip\n";
  draw_tooltip(width, height);
}
