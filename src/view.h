#ifndef USB3SUN_VIEW_H
#define USB3SUN_VIEW_H

#include <bitset>
#include <cstddef>
#include <cstdint>

#include "usb.h"

struct DvChange {
  uint8_t usbkModifier;
  bool make;
};

struct SelChange {
  uint8_t usbkSelector;
  bool make;
};

struct UsbkChanges {
  UsbkReport kreport{};
  DvChange dv[8 * 2]{};
  SelChange sel[6 * 2]{};
  size_t dvLen = 0;
  size_t selLen = 0;
};

struct View {
  virtual void handlePaint() = 0;
  virtual void handleKey(const UsbkChanges &) = 0;

  static View *peek();
  static void push(View *);
  static void pop();
  static void paint();
  static void sendKeys(const UsbkChanges &);
  static void sendMakeBreak(std::bitset<8> usbkModifiers, uint8_t usbkSelector);
};

#endif
