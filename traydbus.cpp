
/*
 * Copyright 2022 P.L. Lucas <selairi@gmail.com>
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
#include "traydbus.h"
#include <cairo.h>

TrayDBus::TrayDBus()
{
  m_slot = nullptr;
  m_bus = nullptr;
}


TrayDBus::~TrayDBus()
{
  finish();
}


void TrayDBus::finish() {
  sd_bus_slot_unref(m_slot);
  sd_bus_unref(m_bus);
}


static int get_property(sd_bus *bus, const char *path, const char *interface, const char *property, sd_bus_message *reply, void *userdata, sd_bus_error *ret_error) {
  int r;
  printf("Property: %s\n", property);
  if(! strcmp("ProtocolVersion", property))
    r = sd_bus_message_append(reply, "u", 0);
  return r;
}


static const sd_bus_vtable watcher_vtable[] = {
  SD_BUS_VTABLE_START(0),
  SD_BUS_PROPERTY("ProtocolVersion", "u", get_property, 0, SD_BUS_VTABLE_PROPERTY_CONST),
  SD_BUS_VTABLE_END
};


void TrayDBus::init()
{
  debug << "Starting DBus" << std::endl;
  sd_bus_error error = SD_BUS_ERROR_NULL;
  sd_bus_message *m = nullptr;
  m_interface_name = "org.kde.StatusNotifierHost";
  m_interface_name += std::to_string(getpid());
  int r = sd_bus_open_user(&m_bus);
  if (r < 0) {
    finish();
    debug << "DBus error. Failed to connect to system bus: " << strerror(-r) << std::endl;
    throw debug_get_func + std::string("DBus error. Failed to connect to system bus: ") + std::string(strerror(-r));
  }

  debug << "DBus has been started." << std::endl;

  // Init StatusNotifierHost service.
  r = sd_bus_add_object_vtable(m_bus,
      &m_slot,
      "/StatusNotifierHost",  // object path 
      m_interface_name.c_str(),   // interface name 
      watcher_vtable,
      this);
  if(r < 0) {
    finish();
    debug << "DBus error. " << m_interface_name << " Failed to issue method call: " << strerror(-r) << std::endl;
    throw debug_get_func + std::string("DBus error. Failed to issue method call: ") + std::string(strerror(-r));
  }

  r = sd_bus_request_name(m_bus, m_interface_name.c_str(), 0);
  if (r < 0) {
    finish();
    debug << "DBus error. Failed to acquire service name: " << strerror(-r) << std::endl;
    throw debug_get_func + std::string("DBus error. Failed to acquire service name :") + std::string(strerror(-r));
  }

  // Check for StatusNotifierWatcher
  {
    int res; 
    r = sd_bus_get_property_trivial(m_bus, 
        "org.kde.StatusNotifierWatcher",
        "/StatusNotifierWatcher",
        "org.kde.StatusNotifierWatcher",
        "IsStatusNotifierHostRegistered",
        &error, 'b', &res
        );
    if(r < 0) {
      finish();
      debug << "DBus error. Failed to connect to StatusNotifierWatcher: " << strerror(-r) << std::endl;
      throw debug_get_func + std::string("DBus error. Failed to connect to StatusNotifierWatcher: ") + std::string(strerror(-r));
    }
    if(res == 0) {
      // Register StatusNotifierHost
      r = sd_bus_call_method(m_bus,
          "org.kde.StatusNotifierWatcher",
          "/StatusNotifierWatcher",
          "org.kde.StatusNotifierWatcher",
          "RegisterStatusNotifierHost",
          &error,
          &m,
          "s",
          m_interface_name.c_str());
      if(r < 0) {
        finish();
        debug << "DBus error. Failed to connect to RegisterStatusNotifierHost: " << strerror(-r) << std::endl;
        throw debug_get_func + std::string("DBus error. Failed to connect to StatusNotifierWatcher: ") + std::string(strerror(-r));
      }
    }
  }
  // Get RegisteredStatusNotifierItems
  {
    char **res;
    r = sd_bus_get_property_strv(m_bus, 
        "org.kde.StatusNotifierWatcher",
        "/StatusNotifierWatcher",
        "org.kde.StatusNotifierWatcher",
        "RegisteredStatusNotifierItems",
        &error, &res);
    for(char **p = res; p != nullptr && *p != nullptr; p++) {
      char *item = *p;
      printf("Item: %s\n", item);
      std::string title = get_icon_title(item);
      printf("Item title: %s\n", title.c_str());
      std::string icon_name = get_icon_name(item);
      printf("Item icon_name: %s\n", icon_name.c_str());
      //icon_activate(item, 0, 0);
      //int32_t w, h;
      //uint8_t *bytes;
      //get_icon_pixmap(item, 32, &w, &h, &bytes);
      //printf("w: %d h: %d\n", w, h);
      add_tray_icon(item);
    }

    // Add and remove new icons
    add_listener_full(
        "org.kde.StatusNotifierWatcher",
        "/StatusNotifierWatcher",
        "org.kde.StatusNotifierWatcher",
        "StatusNotifierItemRegistered",
        [=](const std::string &item_name) {
            add_tray_icon(item_name);
          }
        );
    add_listener_full(
        "org.kde.StatusNotifierWatcher",
        "/StatusNotifierWatcher",
        "org.kde.StatusNotifierWatcher",
        "StatusNotifierItemUnregistered",
        [=](const std::string &item_name) {
            remove_tray_icon(item_name);
          }
        );
  }

  r = 0;
  while(r == 0) {
    r = sd_bus_process(m_bus, nullptr);
    if (r < 0) {
      finish();
      debug << std::string("DBus error. Failed to process bus: ")+ std::string(strerror(-r)) << std::endl;
      throw debug_get_func + std::string("DBus error. Failed to process bus: ")+ std::string(strerror(-r));
    }
  }
  r = sd_bus_wait(m_bus, (uint64_t) -1);
}

int TrayDBus::get_fd()
{
  if(m_bus != nullptr) {
    int fd = sd_bus_get_fd(m_bus);
    return fd;
  }
  return -1;
}

void TrayDBus::init_struct_pollfd(struct pollfd &fds)
{
  if(m_bus != nullptr) {
    fds.fd = sd_bus_get_fd(m_bus);
    int events = sd_bus_get_events(m_bus);
    if(events >= 0)
      fds.events = POLLIN; //sd_bus_get_events(m_bus);
    else {
      debug << "DBus events cannot be read." << std::endl;
      throw debug_get_func + "DBus events cannot be read.";
    }
    fds.revents = 0;
  }
}


void TrayDBus::process_poll_event(struct pollfd &fds)
{
  // Process DBus events
  int r = sd_bus_process(m_bus, nullptr);
  if (r < 0) {
    finish();
    debug << std::string("DBus error. Failed to process bus: ")+ std::string(strerror(-r)) << std::endl;
    throw debug_get_func + std::string("DBus error. Failed to process bus: ")+ std::string(strerror(-r));
  }
  fds.events = sd_bus_get_events(m_bus);
  fds.revents = 0;
}

static std::string get_icon_dbus_name_destination(const std::string &icon_dbus_name)
{
  size_t pos = icon_dbus_name.find_first_of("/");
  return pos != std::string::npos ? icon_dbus_name.substr(0, pos) : std::string();
}

static std::string get_icon_dbus_name_path(const std::string &icon_dbus_name)
{
  size_t pos = icon_dbus_name.find_first_of("/");
  return pos != std::string::npos ? icon_dbus_name.substr(pos) : std::string();
}

std::string TrayDBus::get_icon_title(const std::string &icon_dbus_name)
{
  char *res;
  sd_bus_error error = SD_BUS_ERROR_NULL;
  int r = sd_bus_get_property_string(m_bus, 
      get_icon_dbus_name_destination(icon_dbus_name).c_str(),
      get_icon_dbus_name_path(icon_dbus_name).c_str(),
      "org.kde.StatusNotifierItem",
      "Title",
      &error, &res);
  if(r < 0) {
    debug << "DBus error. Failed to connect to " << icon_dbus_name << " StatusNotifierItem: " << strerror(-r) << std::endl;
    remove_tray_icon(icon_dbus_name);
    return std::string();
    //throw debug_get_func + std::string("DBus error. Failed to connect to StatusNotifierItem: ") + std::string(strerror(-r));
  }
  std::string title(res);
  free(res);
  return title;
}

std::string TrayDBus::get_icon_name(const std::string &icon_dbus_name)
{
  char *res;
  sd_bus_error error = SD_BUS_ERROR_NULL;
  int r = sd_bus_get_property_string(m_bus, 
      get_icon_dbus_name_destination(icon_dbus_name).c_str(),
      get_icon_dbus_name_path(icon_dbus_name).c_str(),
      "org.kde.StatusNotifierItem",
      "IconName",
      &error, &res);
  if(r < 0) {
    debug << "DBus error. Failed to connect to " << icon_dbus_name << " StatusNotifierItem: " << strerror(-r) << std::endl;
    remove_tray_icon(icon_dbus_name);
    return std::string();
    //throw debug_get_func + std::string("DBus error. Failed to connect to StatusNotifierItem: ") + std::string(strerror(-r));
  }
  std::string title(res);
  free(res);
  return title;
}

void TrayDBus::icon_activate(const std::string &icon_dbus_name, int32_t x, int32_t y)
{
  sd_bus_message *m = nullptr;
  sd_bus_error error = SD_BUS_ERROR_NULL;
  int r = sd_bus_call_method(m_bus,
      get_icon_dbus_name_destination(icon_dbus_name).c_str(),
      get_icon_dbus_name_path(icon_dbus_name).c_str(),
      "org.kde.StatusNotifierItem",
      "Activate",
      &error,
      &m,
      "ii",
      x, y);

  if(r < 0) {
    debug << "DBus error. Failed to connect to " << icon_dbus_name << " StatusNotifierItem: " << strerror(-r) << std::endl;
    remove_tray_icon(icon_dbus_name);
    //throw debug_get_func + std::string("DBus error. Failed to connect to StatusNotifierItem: ") + std::string(strerror(-r));
  }
}

void TrayDBus::icon_context_menu(const std::string &icon_dbus_name, int32_t x, int32_t y)
{
  sd_bus_message *m = nullptr;
  sd_bus_error error = SD_BUS_ERROR_NULL;
  int r = sd_bus_call_method(m_bus,
      get_icon_dbus_name_destination(icon_dbus_name).c_str(),
      get_icon_dbus_name_path(icon_dbus_name).c_str(),
      "org.kde.StatusNotifierItem",
      "ContextMenu",
      &error,
      &m,
      "ii",
      x, y);

  if(r < 0) {
    debug << "DBus error. Failed to connect to " << icon_dbus_name << " StatusNotifierItem: " << strerror(-r) << std::endl;
    remove_tray_icon(icon_dbus_name);
    // throw debug_get_func + std::string("DBus error. Failed to connect to StatusNotifierItem: ") + std::string(strerror(-r));
  }
}

bool TrayDBus::get_tooltip(const std::string &icon_dbus_name, std::string &title, std::string &text)
{
  sd_bus_message *m = nullptr;
  sd_bus_error error = SD_BUS_ERROR_NULL;
  int r = sd_bus_get_property(m_bus,
      get_icon_dbus_name_destination(icon_dbus_name).c_str(),
      get_icon_dbus_name_path(icon_dbus_name).c_str(),
      "org.kde.StatusNotifierItem",
      "ToolTip",
      &error,
      &m,
      "(sa(iiay)ss)");

  if(r < 0) {
    debug_error << "DBus error. Failed to connect to " << icon_dbus_name << " ToolTip: " << strerror(-r) << std::endl;
    return false;
  }
  // Open reply as array of type "a(iiay)"
  r = sd_bus_message_enter_container(m, SD_BUS_TYPE_STRUCT, "sa(iiay)ss");
  char *buffer_text = nullptr;
  r = sd_bus_message_read(m, "s", &buffer_text);
  r = sd_bus_message_enter_container(m, SD_BUS_TYPE_ARRAY, "iiay");
  int32_t w, h;
  r = sd_bus_message_read(m, "ii", &w, &h);
  r = sd_bus_message_enter_container(m, SD_BUS_TYPE_ARRAY, "y");
  int size = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, w) * h;
  if(w < 0 || h < 0) size = 0;
  for(int n = 0; n < size && r > 0; n++) {
    int8_t byte;
    r = sd_bus_message_read(m, "y", &byte);
  }
  buffer_text = nullptr;
  r = sd_bus_message_read(m, "s", &buffer_text);
  if(buffer_text != nullptr) title = std::string(buffer_text);
  buffer_text = nullptr;
  r = sd_bus_message_read(m, "s", &buffer_text);
  if(buffer_text != nullptr) text = std::string(buffer_text);
  return true;
}

bool TrayDBus::get_icon_pixmap(const std::string &icon_dbus_name, int32_t prefered_size, int32_t *width, int32_t *height, uint8_t **bytes)
{
  *width = *height = -1;
  *bytes = nullptr;
  sd_bus_message *m = nullptr;
  sd_bus_error error = SD_BUS_ERROR_NULL;
  int r = sd_bus_get_property(m_bus,
      get_icon_dbus_name_destination(icon_dbus_name).c_str(),
      get_icon_dbus_name_path(icon_dbus_name).c_str(),
      "org.kde.StatusNotifierItem",
      "IconPixmap",
      &error,
      &m,
      "a(iiay)");

  if(r < 0) {
    debug_error << "DBus error. Failed to connect to " << icon_dbus_name << " IconPixmap: " << strerror(-r) << std::endl;
    remove_tray_icon(icon_dbus_name);
    return false;
    //throw debug_get_func + std::string("DBus error. Failed to connect to StatusNotifierItem: ") + std::string(strerror(-r));
  }
  // Open reply as array of type "a(iiay)"
  r = sd_bus_message_enter_container(m, SD_BUS_TYPE_ARRAY, "(iiay)");
  // Read array elements
  while(sd_bus_message_enter_container(m, SD_BUS_TYPE_STRUCT, "iiay") >= 0) {
    int32_t w, h, min;
    r = sd_bus_message_read(m, "ii", &w, &h);
    min = w > h ? h : w;
    if(r >= 0) {
      r = sd_bus_message_enter_container(m, SD_BUS_TYPE_ARRAY, "y");
      int size = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, w) * h;
      uint8_t *array_bytes = (uint8_t *) malloc(size);
      for(int n = 0; n < size; n+=4) {
        uint8_t alpha, r, g, b;
        r = sd_bus_message_read(m, "yyyy", &alpha, &r, &g, &b);
        array_bytes[n] = r;
        array_bytes[n + 1] = g;
        array_bytes[n + 2] = b;
        array_bytes[n + 3] = alpha;
      }
      int min_selected_size = *width > *height ? *height : *width;
      if(min >= prefered_size && min_selected_size > min || *width < 0) {
        *width = w;
        *height = h;
        *bytes = array_bytes;
      }
    }
    sd_bus_message_exit_container(m);
  }
  sd_bus_message_unref(m);
  return r >= 0;
}

std::vector<std::string> TrayDBus::get_menu_path(const std::string &icon_dbus_name) 
{
  std::vector<std::string> path;
  sd_bus_message *m = nullptr;
  sd_bus_error error = SD_BUS_ERROR_NULL;
  char *object_path = nullptr;
  int r = sd_bus_get_property(m_bus,
      get_icon_dbus_name_destination(icon_dbus_name).c_str(),
      get_icon_dbus_name_path(icon_dbus_name).c_str(),
      "org.kde.StatusNotifierItem",
      "Menu",
      &error,
      &m,
      "o");
  if(r < 0) {
    debug << "DBus error. Failed to connect to " << icon_dbus_name << " Menu: " << strerror(-r) << std::endl;
  } else {
    r = sd_bus_message_read(m, "o", &object_path);
    if(r < 0) {
      debug << "DBus error. Failed read message connect to " << icon_dbus_name << " Menu: " << strerror(-r) << std::endl;
    } else {
      path.push_back(get_icon_dbus_name_destination(icon_dbus_name));
      path.push_back(std::string(object_path));
    }
  }
  return path;
}

struct SignalHandlerData {
  std::function<void(const std::string &)> handler;
};

static int dbus_signal_handler(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
  printf("[dbus_signal_handler]\n");
  char *arg = nullptr;
  SignalHandlerData *data = (SignalHandlerData *) userdata;
  int r = sd_bus_message_read(m, "s", &arg);
  if(r < 0) {
    debug << "DBus error. Failed read message " << strerror(-r) << std::endl;
  }
  data->handler(arg == nullptr ? std::string() : arg);
  return 0;
}

bool TrayDBus::add_listener_full(const std::string &destination, const std::string &path, const std::string interface, const std::string &signal_name, std::function<void(const std::string &)> handler)
{
  printf("Destination: %s Path: %s\n", destination.c_str(), path.c_str());
  SignalHandlerData *signal_handler_data = new SignalHandlerData();
  signal_handler_data->handler = handler;
  int r = sd_bus_match_signal(m_bus, &m_slot, 
      destination.c_str(),
      path.c_str(), 
      interface.c_str(),
      signal_name.c_str(),
      dbus_signal_handler,
      (void *)signal_handler_data
  );
  if(r < 0) {
    debug_error << "DBus error. Failed connect to signal " << signal_name << " for "<< destination << "/" << path << " Error: " << strerror(-r) << std::endl;
    return false;
  }
  return true;
}

bool TrayDBus::add_listener(const std::string &icon_dbus_name, const std::string &signal_name, std::function<void(const std::string &)> handler)
{
  return add_listener_full(
      get_icon_dbus_name_destination(icon_dbus_name).c_str(), 
      get_icon_dbus_name_path(icon_dbus_name).c_str(),
      "org.kde.StatusNotifierItem",
      signal_name, handler
    );
}
