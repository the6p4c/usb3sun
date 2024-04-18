#include "config.h"
#include "settings.h"

#include "hal.h"

void Settings::begin() {
  if (usb3sun_fs_init()) {
    Sprintln("settings: mounted");
  }
}

void Settings::readAll() {
#ifdef WIPE_SETTINGS
  usb3sun_fs_wipe();
#endif
  read(settings.clickDuration_field);
  read(settings.forceClick_field);
  read(settings.mouseBaud_field);
  read(settings.hostid_field);
}
