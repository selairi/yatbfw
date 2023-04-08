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
 
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string>
#include <systemd/sd-bus.h>
#include <signal.h>
#include <map>

/** This a simple list to add or remove items. The last element is always null.
 */
struct Items {
  char **items; // Array of items. The 
  char items_length; // Length of the items array

  Items() {
    items = (char **) malloc(sizeof(char*));
    items[0] = nullptr;
    items_length = 1;
  }

  ~Items() {
    for(int n = 0; n < items_length; n++) {
      char *item = items[n];
      if(item) free(item);
    }
    free(items);
  }

  void append(char *item) {
    if(! in(item)) {
      items_length++;
      char **p = (char **) realloc(items, sizeof(char *) * (uint8_t)items_length);
      if(p != nullptr) items = p;
      items[items_length - 2] = item;
      items[items_length - 1] = nullptr;
    }
  }

  /** Remove item from array. Item should be deleted.
   */
  void remove(char *item) {
    for(int n = 0; n < items_length; n++) {
      if(items[n] != nullptr) {
        if(item == items[n] || !strcmp(items[n], item)) {
          items_length--;
          for(int i = n; i < items_length; i++) items[i] = items[i + 1];
          char **p = (char **) realloc(items, sizeof(char *) * (uint8_t)items_length);
          if(p != nullptr) items = p;
          break;
        }
      }
    }
  }

  bool in(char *item) {
    for(int n = 0; n < items_length; n++) {
      if(items[n] != nullptr && (items[n] == item || !strcmp(item, items[n])))
        return true;
    }
    return false;
  }
};

struct DBusData {
  Items items;
  char *host_name;
  sd_bus_track *track_item, *track_host;
  sd_bus *bus;
  std::map<std::string,sd_bus_message *> sender_map;

  DBusData() {
    bus = nullptr;
    track_item = track_host = nullptr;
    host_name = nullptr;
  }

  bool is_host_name_registered() {
    return host_name != nullptr;
  }
};

/** Handler to remove item from DBusData::items when DBus object is delected.
 */
static int track_item_handler(sd_bus_track * /*track*/, void *userdata) {
  DBusData *dbus_data = (DBusData *) userdata;
  printf("Track Item handler: ");
  if(dbus_data->track_item) {
    for(int n = 0; n < dbus_data->items.items_length; n++) {
      if(dbus_data->items.items[n] != nullptr) {
        char *item_name = dbus_data->items.items[n];
        int count = sd_bus_track_count_sender(dbus_data->track_item, dbus_data->sender_map[item_name]);
        printf("%s\t%d\n", item_name, count);
        if(count <= 0) {
          dbus_data->items.remove(item_name); 
          sd_bus_emit_properties_changed(dbus_data->bus, "/StatusNotifierWatcher", "org.kde.StatusNotifierWatcher", "RegisteredStatusNotifierItems", NULL);
          sd_bus_emit_signal(dbus_data->bus, "/StatusNotifierWatcher", "org.kde.StatusNotifierWatcher", "StatusNotifierItemUnregistered", "s", item_name);
          printf("StatusNotifierHostUnregistered %s\n", item_name);
          item_name = nullptr; // item_name will be removed by sd_bus
        }
      }
    }
  }
  return 1;
}

/** Handler to remove host from DBusData::host_name when DBus object is delected.
 */
static int track_host_handler(sd_bus_track * /*track*/, void *userdata) {
  DBusData *dbus_data = (DBusData *) userdata;
  printf("Track Host handler: ");
  if(dbus_data->track_host && dbus_data->is_host_name_registered()) {
    int count = sd_bus_track_count_name(dbus_data->track_host, dbus_data->host_name);
    printf("%s\t%d\n", dbus_data->host_name, count);
    if(count <= 0) {
      char *host_name = dbus_data->host_name;
      printf("NotifierHost has been unregistered: %s\n", host_name);
      dbus_data->host_name = nullptr; // host_name will be removed by sd_bus
      sd_bus_emit_properties_changed(dbus_data->bus, "/StatusNotifierWatcher", "org.kde.StatusNotifierWatcher", "IsStatusNotifierHostRegistered", NULL);
      sd_bus_emit_signal(dbus_data->bus, "/StatusNotifierWatcher", "org.kde.StatusNotifierWatcher", "StatusNotifierHostUnregistered", "s", host_name);
    }
  }
  return 1;
}

static int handle_register_status_notifier_item(sd_bus_message *m, void *userdata, sd_bus_error * /*ret_error*/) {
  DBusData *dbus_data = (DBusData *) userdata;
  char *item_name = nullptr;
  int r = sd_bus_message_read(m, "s", &item_name);
  if (r < 0) {
    fprintf(stderr, "Failed to parse parameters: %s\n", strerror(-r));
    return r;
  }
  const char *sender = sd_bus_message_get_sender(m);
  std::string item;
  if(!strcmp(sender, item_name)) {
    item = item_name;
    item += "/StatusNotifierItem";
  } else {
    item = sender;
    item +=item_name;
  }
  char *item_ptr = strdup(item.c_str());
  dbus_data->items.append(item_ptr);

  printf("RegisterStatusNotifierItem: %s -- Sender: %s\n", item_name, sd_bus_message_get_sender(m));

  // Track object
  if(! dbus_data->track_item) {
    sd_bus_track *track;
    int track_ok = sd_bus_track_new(dbus_data->bus, &track, track_item_handler, userdata);
    if(track_ok >= 0) {
      dbus_data->track_item = track;  
    }
  }
  if(dbus_data->track_item) {
    sd_bus_track_add_sender(dbus_data->track_item, m);
    dbus_data->sender_map[item] = m;
  }

  sd_bus_emit_properties_changed(dbus_data->bus, "/StatusNotifierWatcher", "org.kde.StatusNotifierWatcher", "RegisteredStatusNotifierItems", NULL);
  r = sd_bus_emit_signal(dbus_data->bus, "/StatusNotifierWatcher", "org.kde.StatusNotifierWatcher", "StatusNotifierItemRegistered", "s", item_ptr);
  if(r < 0) {
    fprintf(stderr, "Failed to send StatusNotifierItemRegistered signal. Error: %s\n", strerror(-r));
    return r;
  }

  return sd_bus_reply_method_return(m, "");
}


static int handle_register_status_notifier_host(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
  DBusData *dbus_data = (DBusData *) userdata;
  char *item_name = nullptr;
  int r = sd_bus_message_read(m, "s", &item_name);
  if (r < 0) {
    fprintf(stderr, "Failed to parse parameters: %s\n", strerror(-r));
    return r;
  }

  printf("RegisterStatusNotifierHost: %s\n", item_name);
  
  if(dbus_data->is_host_name_registered()) {
    sd_bus_error_set_const(ret_error, "org.kde.StatusNotifierWatcher", "Sorry, a host has already been registered.");
    return -EINVAL; 
  }

  dbus_data->host_name = item_name;
  // Track object
  if(! dbus_data->track_host) {
    sd_bus_track *track;
    int track_ok = sd_bus_track_new(dbus_data->bus, &track, track_host_handler, userdata);
    if(track_ok >= 0) {
      dbus_data->track_host = track;  
    }
  }
  if(dbus_data->track_host) {
    sd_bus_track_add_name(dbus_data->track_host, item_name);
  }

  sd_bus_emit_properties_changed(dbus_data->bus, "/StatusNotifierWatcher", "org.kde.StatusNotifierWatcher", "IsStatusNotifierHostRegistered", NULL);
  sd_bus_emit_signal(dbus_data->bus, "/StatusNotifierWatcher", "org.kde.StatusNotifierWatcher", "StatusNotifierHostRegistered", "", nullptr);

  return sd_bus_reply_method_return(m, "");
}

static int get_property(sd_bus  * /*bus*/, const char * /*path*/, const char * /*interface*/, const char *property, sd_bus_message *reply, void *userdata, sd_bus_error * /*ret_error*/) {
  DBusData *dbus_data = (DBusData *) userdata;
  int r = 1;
  printf("Property: %s\n", property);
  if(! strcmp("ProtocolVersion", property))
    r = sd_bus_message_append(reply, "u", 0);
  else if(! strcmp("IsStatusNotifierHostRegistered", property))
    r = sd_bus_message_append(reply, "b", dbus_data->is_host_name_registered() ? 1 : 0);
  else if(! strcmp("RegisteredStatusNotifierItems", property))
    r = sd_bus_message_append_strv(reply, dbus_data->items.items);
  printf("IsStatusNotifierHostRegistered value: %d - ", dbus_data->is_host_name_registered() ? 1 : 0);
  printf("Error value: %d\n", r);
  if (r < 0) {
    fprintf(stderr, "Failed to parse parameters: %s\n", strerror(-r));
    return r;
  }
  return r;
}

/* The vtable of our little object, implements the net.poettering.Calculator interface */
static const sd_bus_vtable watcher_vtable[] = {
  SD_BUS_VTABLE_START(0),
  SD_BUS_METHOD("RegisterStatusNotifierItem", "s", "", handle_register_status_notifier_item, SD_BUS_VTABLE_UNPRIVILEGED),
  SD_BUS_METHOD("RegisterStatusNotifierHost", "s", "", handle_register_status_notifier_host, SD_BUS_VTABLE_UNPRIVILEGED),
  SD_BUS_PROPERTY("ProtocolVersion", "u", get_property, 0, SD_BUS_VTABLE_PROPERTY_CONST),
  SD_BUS_PROPERTY("IsStatusNotifierHostRegistered", "b", get_property, 0, SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE),
  SD_BUS_PROPERTY("RegisteredStatusNotifierItems", "as", get_property, 0, SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE),
  SD_BUS_SIGNAL("StatusNotifierItemRegistered", "s", 0),
  SD_BUS_SIGNAL("StatusNotifierItemUnregistered", "s", 0),
  SD_BUS_SIGNAL("StatusNotifierHostRegistered", "", 0),
  SD_BUS_SIGNAL("StatusNotifierHostUnregistered", "", 0),
  SD_BUS_VTABLE_END
};

static void finish(sd_bus *bus, sd_bus_slot *slot) {
  sd_bus_slot_unref(slot);
  sd_bus_unref(bus);
  kill(getppid(), SIGCONT);
}

int main(int argc, char *argv[]) {
  //int parent_pid = getppid();
  for(int n = 1; n < argc; n++) {
    if(!strcmp(argv[n], "--help")) {
      printf("This is a simple StatusNotifierWatcher daemon.\n");
      return EXIT_SUCCESS;
    } else {
      printf("Unknown option \"%s\". Try --help option for help.\n", argv[n]);
      return EXIT_FAILURE;
    }
  }

  sd_bus_slot *slot = nullptr;
  sd_bus *bus = nullptr;
  DBusData dbus_data;
  int r;

  r = sd_bus_open_user(&bus);
  if (r < 0) {
    fprintf(stderr, "Failed to connect to system bus: %s\n", strerror(-r));
    finish(bus, slot);
    return EXIT_FAILURE;
  }

  dbus_data.bus = bus;

  r = sd_bus_add_object_vtable(bus,
      &slot,
      "/StatusNotifierWatcher",  /* object path */
      "org.kde.StatusNotifierWatcher",   /* interface name */
      watcher_vtable,
      &dbus_data);
  if (r < 0) {
    fprintf(stderr, "Failed to issue method call: %s\n", strerror(-r));
    finish(bus, slot);
    return EXIT_FAILURE;
  }

  // Take a well-known service name so that clients can find us
  r = sd_bus_request_name(bus, "org.kde.StatusNotifierWatcher", 0);
  if (r < 0) {
    fprintf(stderr, "Failed to acquire service name: %s\n", strerror(-r));
    finish(bus, slot);
    return EXIT_FAILURE;
  }

  bool sigcont_to_parent = false;
  for (;;) {
    // Process requests
    r = sd_bus_process(bus, NULL);
    if (r < 0) {
      fprintf(stderr, "Failed to process bus: %s\n", strerror(-r));
      finish(bus, slot);
      return EXIT_FAILURE;
    }

    if(r == 0) {
      if(!sigcont_to_parent) {
        sigcont_to_parent = true;
        if(kill(getppid(), SIGCONT) <0) 
          printf("SIGCONT error: %s", strerror(errno));
        printf("SIGCONT send to parent with pid %d\n", getppid());
      }
      // Wait for the next request to process.
      // sd_bus_wait has to be called when sd_bus_process returns 0.
      r = sd_bus_wait(bus, (uint64_t) -1);
      if (r < 0) {
        fprintf(stderr, "Failed to wait on bus: %s\n", strerror(-r));
        finish(bus, slot);
        return EXIT_FAILURE;
      }
    }
  }

  finish(bus, slot);

  return EXIT_SUCCESS;
}
