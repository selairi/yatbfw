
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

    if(cairo_surface_status(cairo_surface) == CAIRO_STATUS_SUCCESS)
      printf("cairo_surface creado\n");
    else
      fprintf(stderr, "Error: cairo_surface no se puede crear: %s\n", cairo_status_to_string(cairo_surface_status(cairo_surface)));
  }

  Color color = Settings::get_settings()->color();
  Color background_color = Settings::get_settings()->background_color();

  cairo_t *cr = cairo_create(cairo_surface);
  cairo_set_source_rgba (cr, color.red, color.green, color.blue, 0);
  cairo_paint(cr);

  // Se dibuja el marco de la ventana

  if(! update_items_only) {
    cairo_set_source_rgba (cr, background_color.red, background_color.green, background_color.blue, 1.0);
    cairo_rectangle (cr, 0, 0, m_width, m_height);
    cairo_fill(cr);
  }

  // Draw panel items
  uint32_t x_start = 0, x_end = m_width;
  for(PanelItem *item : m_panel_items) {
    uint32_t x = item->is_start_pos() ? x_start : (x_end - item->getWidth());
    item->setPos(x, 0);
    if(update_items_only) {
      if(item->need_repaint()) {
        item->repaint(cr);
        surface.damage(item->getX(), item->getY(), item->getWidth(), item->getHeight());
      }
    } else {
      item->repaint(cr);
    }
    if(item->is_start_pos())
      x_start += item->getWidth();
    else
      x_end = x;
  }
  
  // Toplevel items are drawn centered
  cairo_save(cr);
  cairo_rectangle(cr, x_start, 0, x_end - x_start, m_height);
  cairo_clip(cr);
  uint32_t x_toplevels = 0, n_toplevels = 0;
  for(ToplevelButton *item : m_toplevel_handles) {
    x_toplevels += item->getWidth();
    n_toplevels++;
  }
  // Change toplevel items offset if there is not enoght space 
  // and user moves the mouse wheel
  if(x_toplevels > (x_end - x_start))
    x_toplevels += m_toplevel_items_offset;
  x_toplevels = (x_start + x_end - x_toplevels) / 2;

  for(ToplevelButton *item : m_toplevel_handles) {
    item->setPos(x_toplevels, 0);
    if(update_items_only) {
      if(item->need_repaint()) {
        item->repaint(cr);
        surface.damage(item->getX(), item->getY(), item->getWidth(), item->getHeight());
      }
    } else {
      item->repaint(cr);
    }
    x_toplevels += item->getWidth(); 
  }
  cairo_restore(cr);


  cairo_destroy(cr);

  surface.attach(buffer.at(0), 0, 0);
  if(! update_items_only)
    surface.damage(0, 0, m_width, m_height);

  surface.commit();
  printf("Redibujado\n");
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
}

void Panel::init()
{
  m_width = m_height = Settings::get_settings()->panel_size();
  // retrieve global objects
  registry = display.get_registry();
  registry.on_global() = [&] (uint32_t name, const std::string& interface, uint32_t version)
  {
    std::cout << interface << " version " << version << std::endl;

    if(interface == compositor_t::interface_name)
      registry.bind(name, compositor, version);
    else if(interface == shell_t::interface_name)
      registry.bind(name, shell, version);
    else if(interface == xdg_wm_base_t::interface_name)
      registry.bind(name, xdg_wm_base, version);
    else if(interface == seat_t::interface_name)
      registry.bind(name, seat, version);
    else if(interface == shm_t::interface_name)
      registry.bind(name, shm, version);
    else if(interface == zwlr_layer_shell_v1_t::interface_name)
      registry.bind(name, layer_shell, version);
    else if(interface == zwlr_foreign_toplevel_manager_v1_t::interface_name) {
      printf("Toplevel version: %d\n", version);
      registry.bind(name, toplevel_manager, version);
      printf("Toplevel version: %d\n", version);
      toplevel_manager.on_toplevel() = [&](zwlr_foreign_toplevel_handle_v1_t handle) {
        std::cout << "  toplevel_manager::on_toplevel" << std::endl;
        on_toplevel_listener(handle);
      };
    } else if(interface == output_t::interface_name)
      registry.bind(name, output, version);

  };
  display.roundtrip();

  seat.on_capabilities() = [&] (const seat_capability& capability)
  {
    has_keyboard = capability & seat_capability::keyboard;
    has_pointer = capability & seat_capability::pointer;
  };


  // create a surface
  surface = compositor.create_surface();

  // create a shell surface
  if(layer_shell) {
    layer_shell_surface = layer_shell.get_layer_surface(surface, output, zwlr_layer_shell_v1_layer::top, std::string("Window"));
    layer_shell_surface.set_size(m_width, m_height);
    switch(Settings::get_settings()->panel_position()) {
      case PanelPosition::TOP:
        layer_shell_surface.set_anchor(zwlr_layer_surface_v1_anchor::top);
        break;
      default:
        layer_shell_surface.set_anchor(zwlr_layer_surface_v1_anchor::bottom);
    }
    layer_shell_surface.set_exclusive_zone(m_height);
    layer_shell_surface.set_keyboard_interactivity(zwlr_layer_surface_v1_keyboard_interactivity::none);
    layer_shell_surface.on_configure() = [&](uint32_t serial, uint32_t width, uint32_t height) {
      m_width = width;
      m_height = height; 
      layer_shell_surface.set_size(m_width, m_height);
      layer_shell_surface.set_exclusive_zone(m_height);
      layer_shell_surface.ack_configure(serial);
    };
  } else if(xdg_wm_base)
  {
    xdg_wm_base.on_ping() = [&] (uint32_t serial) { xdg_wm_base.pong(serial); };
    xdg_surface = xdg_wm_base.get_xdg_surface(surface);
    xdg_surface.on_configure() = [&] (uint32_t serial) { xdg_surface.ack_configure(serial); };
    xdg_toplevel = xdg_surface.get_toplevel();
    xdg_toplevel.set_title("Window");
    xdg_toplevel.on_close() = [&] () { running = false; };
  }
  else
  {
    shell_surface = shell.get_shell_surface(surface);
    shell_surface.on_ping() = [&] (uint32_t serial) { shell_surface.pong(serial); };
    shell_surface.set_title("Window");
    shell_surface.set_toplevel();
    printf("shell_surface\n");
  }
  surface.commit();

  display.roundtrip();

  // Get input devices
  if(!has_keyboard)
    throw std::runtime_error("No keyboard found.");
  if(!has_pointer)
    throw std::runtime_error("No pointer found.");

  pointer = seat.get_pointer();
  keyboard = seat.get_keyboard();

  // create shared memory
  shared_mem = std::make_shared<shared_mem_t>(2*m_width*m_height*4);
  auto pool = shm.create_pool(shared_mem->get_fd(), 2*m_width*m_height*4);
  for(unsigned int c = 0; c < 2; c++)
    buffer.at(c) = pool.create_buffer(c*m_width*m_height*4, m_width, m_height, m_width*4, shm_format::argb8888);
  cur_buf = 0;

  // load cursor theme
  cursor_theme_t cursor_theme = cursor_theme_t(Settings::get_settings()->cursor_theme(), Settings::get_settings()->cursor_size(), shm);
  cursor_t cursor = cursor_theme.get_cursor("left_ptr");
  cursor_image = cursor.image(0);
  cursor_buffer = cursor_image.get_buffer();

  // create cursor surface
  cursor_surface = compositor.create_surface();

  // draw cursor
  pointer.on_enter() = [&] (uint32_t serial, const surface_t& /*unused*/, int32_t x, int32_t y)
  {
    cursor_surface.attach(cursor_buffer, 0, 0);
    cursor_surface.damage(0, 0, cursor_image.width(), cursor_image.height());
    cursor_surface.commit();
    pointer.set_cursor(serial, cursor_surface, 0, 0);
    printf("Cursor %d,%d\n", x, y);
    m_last_cursor_x = x;
    m_last_cursor_y = y;
    for(PanelItem *item : m_panel_items)
      item->on_mouse_enter(x, y);
    for(ToplevelButton *item : m_toplevel_handles)
      item->on_mouse_enter(x, y);

    m_repaint_partial = true;
    //draw(serial, true);
  };

  pointer.on_leave() = [&] (uint32_t serial, const surface_t& /*unused*/)
  {
    printf("on_leave\n");
    for(PanelItem *item : m_panel_items)
      item->on_mouse_leave(m_last_cursor_x, m_last_cursor_y, true);
    for(ToplevelButton *item : m_toplevel_handles)
      item->on_mouse_leave(m_last_cursor_x, m_last_cursor_y, true);
    m_repaint_partial = true;
    //draw(serial, true);
  };

  pointer.on_motion() = [&] (uint32_t time, double x, double y)
  {
    //printf("Cursor %f,%f\n", x, y);
    m_last_cursor_x = x;
    m_last_cursor_y = y;
    for(PanelItem *item : m_panel_items) {
      item->on_mouse_enter(x, y);
      item->on_mouse_leave(x, y, false);
    }
    for(ToplevelButton *item : m_toplevel_handles) {
      item->on_mouse_enter(x, y);
      item->on_mouse_leave(x, y, false);
    }
    m_repaint_partial = true;
    //draw(-1, true);
  };

  pointer.on_button() = [&] (uint32_t serial, uint32_t /*unused*/, uint32_t button, pointer_button_state state)
  {
    printf("Botón pulsado %d\n", button);
    if(/*(button == BTN_LEFT || button == BTN_RIGHT) && */state == pointer_button_state::pressed) {
      printf("Botón pulsado\n");
      for(PanelItem *item : m_panel_items)
        item->on_mouse_clicked(m_last_cursor_x, m_last_cursor_y, button);
      for(ToplevelButton *item : m_toplevel_handles)
        ((PanelItem*)item)->on_mouse_clicked(m_last_cursor_x, m_last_cursor_y, button);
      m_repaint_partial = true;
      //draw(serial, true);
    } else if(/*(button == BTN_LEFT || button == BTN_RIGHT) && */state != pointer_button_state::pressed) {
      for(PanelItem *item : m_panel_items)
        item->on_mouse_released(m_last_cursor_x, m_last_cursor_y);
      for(ToplevelButton *item : m_toplevel_handles)
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
  draw();
  draw();
}


void Panel::on_toplevel_listener(zwlr_foreign_toplevel_handle_v1_t toplevel_handle)
{
  ToplevelButton *toplevel = new ToplevelButton(toplevel_handle, seat, &m_toplevel_handles);
  toplevel->setWidth(Settings::get_settings()->panel_size());
  toplevel->setHeight(Settings::get_settings()->panel_size());
  toplevel->repaint_main_interface = [&](bool update_items_only) {
    //draw(-1, update_items_only);
    if(update_items_only)
      m_repaint_partial = true;
    else
      m_repaint_full = true;
  };
  if(! toplevel)
    std::cerr << "Error: No free memory" << std::endl;
  else
    m_toplevel_handles.push_back(toplevel);
  //draw(-1, false);
  m_repaint_full = true;
}


void Panel::addLauncher(std::string icon, std::string text, std::string exec, bool start_pos)
{
  ButtonRunCommand *n = new ButtonRunCommand((char*)(icon.c_str()), text);
  n->setCommand(exec);
  n->setFD(display.get_fd());
  n->setWidth(Settings::get_settings()->panel_size() - 1);
  n->setHeight(Settings::get_settings()->panel_size() - 1);
  n->set_start_pos(start_pos);
  m_panel_items.push_back(n);
}

void Panel::addClock(std::string icon, std::string format, std::string exec, bool start_pos)
{
  Clock *c = new Clock((char*)(icon.c_str()), format);
  c->setWidth(Settings::get_settings()->panel_size() - 1);
  c->setHeight(Settings::get_settings()->panel_size() - 1);
  c->setCommand(exec);
  c->send_repaint = [&]() {
    //draw(-1, true);
    m_repaint_partial = true;
  };
  c->setFD(display.get_fd());
  c->set_start_pos(start_pos);
  m_panel_items.push_back(c);
}

void Panel::addBattery(std::string exec, bool start_pos)
{
  Battery *c = new Battery();
  c->setWidth(Settings::get_settings()->panel_size() - 1);
  c->setHeight(Settings::get_settings()->panel_size() - 1);
  c->setCommand(exec);
  c->send_repaint = [&]() {
    //draw(-1, true);
    m_repaint_partial = true;
  };
  c->setFD(display.get_fd());
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
    for(PanelItem *item : m_panel_items) {
      long item_timeout = item->next_time_timeout(now_in_msecs);
      //printf("\n item_timeout %ld\n", item_timeout);
      //printf(" now_in_msecs %ld\n", now_in_msecs);
      if(item_timeout >= 0) {
        if(now_in_msecs >= item_timeout) {
          item->on_timeout(now_in_msecs);
          item_timeout = item->next_time_timeout(now_in_msecs);
          //printf(" item_timeout %ld event run\n", item_timeout);
        }
        long timeout = item_timeout - now_in_msecs;
        if(timeout_msecs < 0 || timeout < timeout_msecs)
          timeout_msecs = timeout;
      }
    }
    printf("Timeout %d\n", timeout_msecs);
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
      printf("  Timeout\n");
      //for(PanelItem *item : m_panel_items) {
      //  item->on_timeout();
      //}
    } else {
      printf("  poll failed %d\n", ret);
    }
  }
}


