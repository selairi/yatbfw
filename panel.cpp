
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
#include <limits.h>
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
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <time.h>

#include <cairo/cairo.h>

#include "layer-shell.h"
#include "toplevel.h"
#include "buttonruncommand.h"
#include "tray.h"
#include "traybutton.h"
#include "clock.h"
#include "battery.h"
#include "panel.h"
#include "settings.h"
#include "utils.h"

#define WIDTH 34
#define HEIGHT 34

using namespace wayland;

#include "shared_mem.hpp"

/** This class deletes the cairo_t *cr pointer.
 */
class Guard_cairo_t {
  cairo_t *cr;
public:
  Guard_cairo_t(cairo_t *cr) { this->cr = cr; }
  ~Guard_cairo_t() { cairo_destroy(cr); }
};

void Panel::draw(uint32_t serial, bool update_items_only)
{
  if(!surface || !shared_mem)
    return;

  if(cairo_surface == nullptr) {
    if(shared_mem->get_mem() != nullptr)
      cairo_surface = cairo_image_surface_create_for_data((unsigned char*)(shared_mem->get_mem()), CAIRO_FORMAT_ARGB32, (int) m_width, (int) m_height, 
          // /*stride*/ m_width*4
          cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, (int) m_width)
      );
    else
      throw std::string("cairo_surface_status cannot be created.");

    if(cairo_surface_status(cairo_surface) == CAIRO_STATUS_SUCCESS) {
      debug << "New cairo_surface\n";
    } else {
      debug_error << "cairo_surface cannot be created: " 
        << cairo_status_to_string(cairo_surface_status(cairo_surface)) 
        << std::endl;
      throw std::string("cairo_surface cannot be created");
    }
  }

  // Check limits.
  if(m_width > INT_MAX || m_height > INT_MAX || m_toplevel_items_offset > INT_MAX) {
    debug_error << "m_width or m_height is bigger than int size." << std::endl;
    exit(1);
  }
  // Casts from uint32_t to int is safe

  Color color = Settings::get_settings()->color();
  Color background_color = Settings::get_settings()->background_color();

  cairo_t *cr = cairo_create(cairo_surface);
  if(cairo_status(cr) != CAIRO_STATUS_SUCCESS) {
    throw std::string("cairo_t cannot be created.");
  }
  Guard_cairo_t cr_guard(cr);
  cairo_set_source_rgba (cr, color.red, color.green, color.blue, 0);
  cairo_paint(cr);

  // Draw window frame
  if(! update_items_only) {
    cairo_set_source_rgba (cr, background_color.red, background_color.green, background_color.blue, 1.0);
    cairo_rectangle (cr, 0, 0, m_width, m_height);
    cairo_fill(cr);
  }

  // Draw panel items
  int x_start = 0, x_end = (int)m_width;
  for(auto item : m_panel_items) {
    int x = item->is_start_pos() ? x_start : (x_end - item->get_width());
    if(update_items_only) {
      if(item->need_repaint()) {
        int width = item->get_width(), height = item->get_height();
        item->update_size(cr);
        if(width != item->get_width() || height != item->get_height()) {
          // Total repaint is needed
          debug << "Total repaint is needed" << std::endl;
          //cairo_destroy(cr);
          draw(serial, false);
          return;
        }
        item->set_pos((int)x, 0);
        item->repaint(cr);
        surface.damage(item->get_x(), item->get_y(), item->get_width(), item->get_height());
      }
    } else {
      item->update_size(cr);
      x = item->is_start_pos() ? x_start : (x_end - item->get_width());
      item->set_pos(x, 0);
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
  int x_toplevels = 0;
  for(std::shared_ptr<ToplevelButton> item : m_toplevel_handles) {
    x_toplevels += item->get_width();
  }
  // Change toplevel items offset if there is not enoght space 
  // and user moves the mouse wheel
  if(x_toplevels > (x_end - x_start))
    x_toplevels += (int) m_toplevel_items_offset;
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


  //cairo_destroy(cr);

  surface.attach(buffer.at(0), 0, 0);
  if(! update_items_only)
    surface.damage(0, 0, (int32_t)m_width, (int32_t)m_height);

  surface.commit();
  debug << "draw finished\n";
}

Panel::Panel()
{
  m_width  = (uint32_t) Settings::get_settings()->panel_size();
  m_height = (uint32_t) Settings::get_settings()->panel_size();
  m_last_cursor_x = m_last_cursor_y = 0;
  m_toplevel_items_offset = 0;
  cairo_surface = nullptr;
  m_repaint_full = true;
  m_repaint_partial = false;
  m_tray_dbus = nullptr;
  m_tray = nullptr;
  m_popup = nullptr;
}

static void sigcont_handler(int sig)
{
  printf("Signal %d\n", sig);
}

void Panel::init()
{
  if(! m_tray_dbus) {
    // Start StatusNotifyWatcher daemon
    printf("Starting status-notify-watcher-daemon\n");
    pid_t child_pid;
    if((child_pid = fork()) == 0) {
      int exit_value = execlp("status-notify-watcher-daemon", "status-notify-watcher-daemon", nullptr);
      if(exit_value < 0)
        printf("StatusNotifyWatcher error: %d error: %s\n", exit_value, strerror(errno));
      else
        printf("StatusNotifyWatcher is runnig\n");
      exit(0);
    }
    printf("Waiting for signal. My pid %d\n", getpid());
    signal(SIGCONT, sigcont_handler);
    pause();
    printf("StatusNotifyWatcher is running\n");
    // Make a new StatusNotifyWatcher dbus connection
    m_tray_dbus = std::make_shared<TrayDBus>();
    m_tray_dbus->add_tray_icon = [&](const std::string &tray_icon_dbus_name) {
      add_tray_icon(tray_icon_dbus_name, false);
      m_repaint_full = true;
    };
    m_tray_dbus->remove_tray_icon = [&](const std::string &tray_icon_dbus_name) {
      printf("Panel removing icon %s\n", tray_icon_dbus_name.c_str());
      remove_tray_icon(tray_icon_dbus_name);
      printf("Panel removed icon\n");
      printf("Panel removed icon %s\n", tray_icon_dbus_name.c_str());
      m_repaint_full = true;
    };
    m_tray_dbus->init();
  }

  m_width = m_height = (uint32_t)Settings::get_settings()->panel_size();
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
      output.on_mode() = [&](uint32_t /*flags*/, int32_t width, int32_t /*height*/, int32_t /*refresh*/) {
        m_width = (uint32_t)width;
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
        layer_shell_surface.set_anchor(zwlr_layer_surface_v1_anchor::top | zwlr_layer_surface_v1_anchor::right | zwlr_layer_surface_v1_anchor::left);
        break;
      default:
        layer_shell_surface.set_anchor(zwlr_layer_surface_v1_anchor::bottom | zwlr_layer_surface_v1_anchor::right | zwlr_layer_surface_v1_anchor::left);
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
        surface.damage(0, 0, (int32_t)m_width, (int32_t)m_height);
        surface.commit();
      }
      layer_shell_surface.ack_configure(serial);
    };
    layer_shell_surface.on_closed() = [&]() {
      debug_error << "Layer shell has been closed" << std::endl;
      running = false;
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
  const uint32_t pool_size = 2*m_width*m_height*4;
  shared_mem = std::make_shared<shared_mem_t>(pool_size);
  if(pool_size > INT32_MAX) {
    debug_error << "pool_size overflows" << std::endl;
    exit(1);
  }
  auto pool = shm.create_pool(shared_mem->get_fd(), (int32_t)pool_size);
  for(unsigned int c = 0; c < 2; c++)
    buffer.at(c) = pool.create_buffer((int32_t)(c*m_width*m_height*4), (int32_t)m_width, (int32_t)m_height, (int32_t)(m_width*4), shm_format::argb8888);
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
  pointer.on_enter() = [&] (uint32_t serial, const surface_t& surface_entered, int32_t x, int32_t y)
  {
    debug << "Cursor on_enter start:" << std::endl;
    m_pointer_last_surface_entered = surface_entered;
    cursor_surface.attach(cursor_buffer, 0, 0);
    if(cursor_image.width() > INT32_MAX || cursor_image.height() > INT32_MAX) {
      debug_error << "cursor size is too big" << std::endl;
      exit(1);
    }
    cursor_surface.damage(0, 0, (int32_t)cursor_image.width(), (int32_t)cursor_image.height());
    cursor_surface.commit();
    pointer.set_cursor(serial, cursor_surface, 0, 0);
    debug << "Cursor " << x << ", " << y << " m_height: " << m_height << " popup visible: " << std::endl;
    if(x < 0) x = 0;
    if(y < 0) y = 0;
    m_last_cursor_x = x;
    m_last_cursor_y = y;
    if(y <= (int32_t)m_height && surface == surface_entered) {
      debug << "Cursor Items" << std::endl;
      for(auto item : m_panel_items)
        item->on_mouse_enter(x, y);
      debug << "Cursor Toplevels" << std::endl;
      for(std::shared_ptr<ToplevelButton> item : m_toplevel_handles)
        item->on_mouse_enter(x, y);
      m_repaint_partial = true;
    } else if(m_popup != nullptr && m_popup->is_visible() && m_popup->get_surface() == surface_entered) {
      debug << "Cursor popup" << std::endl;
      m_popup->on_mouse_enter(x, y);
    }
    debug << "Cursor on_enter end." << std::endl;
    //draw(serial, true);
  };

  pointer.on_leave() = [&] (uint32_t /*serial*/, const surface_t& surface_left)
  {
    debug << "on_leave\n";
    if(surface == surface_left) {
      for(auto item : m_panel_items)
        item->on_mouse_leave(m_last_cursor_x, m_last_cursor_y, true);
      for(std::shared_ptr<ToplevelButton> item : m_toplevel_handles)
        item->on_mouse_leave(m_last_cursor_x, m_last_cursor_y, true);
      m_repaint_partial = true;
      ToolTip::hide();
    } else if(m_popup != nullptr && m_popup->is_visible() && m_popup->get_surface() == surface_left) {
      m_popup->on_mouse_leave(m_last_cursor_x, m_last_cursor_y, true);
    }
  };

  pointer.on_motion() = [&] (uint32_t /*time*/, double x, double y)
  {
    debug << "on_motion\n";
    //printf("Cursor %f,%f\n", x, y);
    m_last_cursor_x = (int32_t)x;
    m_last_cursor_y = (int32_t)y;
    if(y <= m_height && surface == m_pointer_last_surface_entered) {
      for(auto item : m_panel_items) {
        item->on_mouse_enter((int)x, (int)y);
        item->on_mouse_leave((int)x, (int)y, false);
      }
      for(std::shared_ptr<ToplevelButton> item : m_toplevel_handles) {
        item->on_mouse_enter((int)x, (int)y);
        item->on_mouse_leave((int)x, (int)y, false);
      }
      m_repaint_partial = true;
    } else if(m_popup != nullptr && m_popup->is_visible() && m_pointer_last_surface_entered == m_popup->get_surface()){
      m_popup->on_mouse_enter((int)x, (int)y);
      m_popup->on_mouse_leave((int)x, (int)y, false);
    }
    //draw(-1, true);
  };

  pointer.on_button() = [&] (uint32_t /*serial*/, uint32_t /*unused*/, uint32_t button, pointer_button_state state)
  {
    debug << "Button action  " << button << std::endl;
    if(surface == m_pointer_last_surface_entered) {
      if(/*(button == BTN_LEFT || button == BTN_RIGHT) && */state == pointer_button_state::pressed) {
        debug << "Button pressed\n";
        for(auto item : m_panel_items)
          item->on_mouse_clicked(m_last_cursor_x, m_last_cursor_y, (int)button);
        for(std::shared_ptr<ToplevelButton> item : m_toplevel_handles)
          item->on_mouse_clicked(m_last_cursor_x, m_last_cursor_y, (int)button);
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
    } else if(m_popup != nullptr && m_popup->is_visible() && m_popup->get_surface() == m_pointer_last_surface_entered) {
      if(/*(button == BTN_LEFT || button == BTN_RIGHT) && */state == pointer_button_state::pressed) {
        debug << "Button pressed\n";
        m_popup->on_mouse_clicked(m_last_cursor_x, m_last_cursor_y, (int)button);
      } else if(/*(button == BTN_LEFT || button == BTN_RIGHT) && */state != pointer_button_state::pressed) {
        m_popup->on_mouse_released(m_last_cursor_x, m_last_cursor_y);
      }
    }
  };

  pointer.on_axis() = [&] (uint32_t /*time*/, pointer_axis /*axis*/, double value) {
    // Change toplevel items offset if there is not enoght space 
    // and user moves the mouse wheel
    //if(axis == pointer_axis::vertical_scroll)
    //  printf("on_axis: axis vertical\n");
    //else
    //  printf("on_axis: axis horizontal\n");
    debug << "on_axis\n";
    m_toplevel_items_offset += (value > 0.0 ? 1 : -1) * (int32_t)(Settings::get_settings()->panel_size());
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
  n->set_fd(get_fds());
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
  c->set_fd(get_fds());
  c->set_start_pos(start_pos);
  m_panel_items.push_back(c);
}

void Panel::add_tray(bool start_pos)
{
  m_tray = std::make_shared<Tray>();
  m_tray->set_width(1);
  m_tray->set_height(1);
  m_tray->set_start_pos(start_pos);
  m_panel_items.push_back(m_tray);
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
  c->set_fd(get_fds());
  c->set_start_pos(start_pos);
  m_panel_items.push_back(c);
}

std::vector<int> Panel::get_fds()
{
  std::vector<int> fds;
  fds.push_back(display.get_fd());
  if(m_tray_dbus != nullptr && m_tray_dbus->get_fd() > -1)
    fds.push_back(m_tray_dbus->get_fd());
  return fds;
}
#include <algorithm>
void Panel::add_tray_icon(const std::string &tray_icon_dbus_name, bool start_pos)
{
  if(m_tray == nullptr) 
    return;
  auto it = std::find_if(m_panel_items.begin(), m_panel_items.end(), [this](std::shared_ptr<PanelItem> item) { return (void*)(item.get()) == (void*)(m_tray.get());});
  if(! tray_icon_dbus_name.empty() && tray_icon_dbus_name != "/" && it != m_panel_items.end()) {
    auto c = std::make_shared<TrayButton>(m_tray_dbus, tray_icon_dbus_name);
    c->set_width(Settings::get_settings()->panel_size() - 1);
    c->set_height(Settings::get_settings()->panel_size() - 1);
    c->send_repaint = [&]() {
      m_repaint_partial = true;
    };
    c->new_popup = [&]() -> std::shared_ptr<Popup> {
      ToolTip::hide();
      m_popup = std::make_shared<Popup>(&compositor, &display, &xdg_wm_base, &shm, &layer_shell_surface, &m_width, &m_height);
      return m_popup;
    };
    // tray icon must be inserted at m_tray position
    c->set_start_pos(m_tray != nullptr ? m_tray->is_start_pos() : start_pos);
    m_panel_items.insert(it, c);
    m_panel_tray_icons[tray_icon_dbus_name] = c;
  }
}

void Panel::remove_tray_icon(const std::string &tray_icon_dbus_name)
{
  if(m_panel_tray_icons.find(tray_icon_dbus_name) != m_panel_tray_icons.end()) {
    std::shared_ptr<PanelItem> c = m_panel_tray_icons[tray_icon_dbus_name];
    m_panel_items_delete_later.push_back(c);
    m_panel_tray_icons.erase(tray_icon_dbus_name);
    for(std::vector<std::shared_ptr<PanelItem> >::iterator it = m_panel_items.begin(); it != m_panel_items.end(); it++) {
      if((*it) == c) {
        m_panel_items.erase(it);
        break;
      }
    }
  }
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
  struct pollfd fds[2];
  long timeout_msecs = -1;
  int ret;
  long now_in_msecs;

  fds[0].fd = display.get_fd();
  fds[0].events = POLLIN;
  fds[0].revents = 0;

  if(m_tray_dbus != nullptr)
    m_tray_dbus->init_struct_pollfd(fds[1]);

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
      draw(0, true);
    // Proccess pending Wayland events
    display.dispatch_pending();
    display.flush();
    m_repaint_full = m_repaint_partial = false;
    // Wait for events
    ret = poll(fds, m_tray_dbus != nullptr ? 2 : 1, (int)timeout_msecs);
    if(ret > 0) {
      if(fds[0].revents) {
        // Process Wayland events
        display.dispatch();
        fds[0].revents = 0;
      }
      if(m_tray_dbus != nullptr && fds[1].revents) {
        // Process tray DBus events
        m_tray_dbus->process_poll_event(fds[1]);
      }
    } else if(ret == 0) {
      debug << "Timeout\n";
      //for(auto item : m_panel_items) {
      //  item->on_timeout();
      //}
    } else {
      debug << "poll failed %d" << ret << std::endl;
    }
    // Delete items after drawing operations
    if(! m_panel_items_delete_later.empty()) {
      debug << "Delete items after drawing operations" << std::endl;
      m_panel_items_delete_later.clear();
    }
  }
}

