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
  if (!read<ClickDurationV2>(clickDuration)) {
    ClickDurationV1 v1{};
    if (readV1(v1)) {
      clickDuration = v1.value;
      write<ClickDurationV2>(clickDuration);
    }
  }
  if (!read<ForceClickV2>(forceClick)) {
    ForceClickV1 v1{};
    if (readV1(v1)) {
      forceClick = v1.value;
      write<ForceClickV2>(forceClick);
    }
  }
  if (!read<MouseBaudV2>(mouseBaud)) {
    MouseBaudV1 v1{};
    if (readV1(v1)) {
      mouseBaud = v1.value;
      write<MouseBaudV2>(mouseBaud);
    }
  }
  if (!read<HostidV2>(hostid)) {
    HostidV1 v1{};
    if (readV1(v1)) {
      hostid = v1.value;
      write<HostidV2>(hostid);
    }
  }
}
