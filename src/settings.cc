#include "config.h"
#include "settings.h"

#include "hal.h"

void Settings::begin() {
  if (usb3sun_fs_init()) {
    Sprintln("settings: mounted");
  }
}

void Settings::readAll() {
  read(settings.clickDuration_field);
  read(settings.forceClick_field);
  read(settings.mouseBaud_field);
  read(settings.hostid_field);
}
