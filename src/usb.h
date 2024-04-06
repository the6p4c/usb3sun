#ifndef USB3SUN_USB_H
#define USB3SUN_USB_H

#include <cstdint>

// hid interface protocol
#define USB3SUN_UHID_KEYBOARD 1
#define USB3SUN_UHID_MOUSE 2

struct __attribute__((packed)) UsbkReport {
  uint8_t modifier;
  uint8_t reserved;
  uint8_t keycode[6];
};

struct __attribute__((packed)) UsbmReport {
  uint8_t buttons;
  int8_t x;
  int8_t y;
  int8_t wheel;
  int8_t pan;
};

#endif
