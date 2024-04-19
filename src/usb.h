#ifndef USB3SUN_USB_H
#define USB3SUN_USB_H

#include <cstdint>

#include "bindings.h"

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

const uint16_t ASCII_TO_USBK[128] = {
    /* 00h */ 0, 0, 0, 0, 0, 0, 0, 0,
    /* 08h */ 0, 0, USBK_RETURN, 0, 0, 0, 0, 0,
    /* 10h */ 0, 0, 0, 0, 0, 0, 0, 0,
    /* 18h */ 0, 0, 0, 0, 0, 0, 0, 0,
    /* 20h */ 0, 0, 0, 0, 0, 0, 0, 0,
    /* 28h */ 0, 0, 0, 0, 0, 0, 0, 0,
    /* 30h */ USBK_0, USBK_1, USBK_2, USBK_3, USBK_4, USBK_5, USBK_6, USBK_7,
    /* 38h */ USBK_8, USBK_9, 0, 0, 0, 0, 0, 0,
    /* 40h */ 0, 0, 0, 0, 0, 0, 0, 0,
    /* 48h */ 0, 0, 0, 0, 0, 0, 0, 0,
    /* 50h */ 0, 0, 0, 0, 0, 0, 0, 0,
    /* 58h */ 0, 0, 0, 0, 0, 0, 0, 0,
    /* 60h */ 0, USBK_A, USBK_B, USBK_C, USBK_D, USBK_E, USBK_F, USBK_G,
    /* 68h */ USBK_H, USBK_I, USBK_J, USBK_K, USBK_L, USBK_M, USBK_N, USBK_O,
    /* 70h */ USBK_P, USBK_Q, USBK_R, USBK_S, USBK_T, USBK_U, USBK_V, USBK_W,
    /* 78h */ USBK_X, USBK_Y, USBK_Z, 0, 0, 0, 0, 0,
};

#endif
