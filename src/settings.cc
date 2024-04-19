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
  readV1(settings.clickDuration_field);
  readV1(settings.forceClick_field);
  readV1(settings.mouseBaud_field);
  readV1(settings.hostid_field);
}
