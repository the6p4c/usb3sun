#include "config.h"
#include "hostid.h"

#include <cstdio>
#include <cstring>

#include "bindings.h"
#include "hal.h"

HostidView HOSTID_VIEW{};

const char *HostidView::name() const {
  return "HostidView";
}

void HostidView::handlePaint() {
  char hostid_text[sizeof newHostid + 1];
  snprintf(
    hostid_text, sizeof hostid_text, "%c%c%c%c%c%c",
    newHostid.value[0], newHostid.value[1], newHostid.value[2],
    newHostid.value[3], newHostid.value[4], newHostid.value[5]);
  hostid_text[sizeof hostid_text - 1] = '\0';

  usb3sun_display_text(8, 8, false, "Hostid:");
  usb3sun_display_text(8 + 6 * cursorIndex, 18, false, "_");
  usb3sun_display_text(8, 16, false, hostid_text);
  usb3sun_display_vline(8 + 6 * 8 - 1, 0, 32, false, false);
  usb3sun_display_hline(8 + 6 * 8 - 1 + 4, 8 + 6, 6 * 11 - 1, false, 3);
  usb3sun_display_hline(8 + 6 * 8 - 1 + 4, 16 + 6, 6 * 11 - 1, false, 3);
  usb3sun_display_text(8 + 6 * 8 - 1 + 4, 8, false, "ENTER\377\377\377\377ok", true);
  usb3sun_display_text(8 + 6 * 8 - 1 + 4, 16, false, "ESC\377\377cancel", true);
}

void HostidView::handleKey(const UsbkChanges &changes) {
  for (size_t i = 0; i < changes.selLen; i++) {
    if (changes.sel[i].make) {
      switch (changes.sel[i].usbkSelector) {
        case USBK_RETURN:
        case USBK_ENTER:
          ok();
          break;
        case USBK_ESCAPE:
          cancel();
          break;
        case USBK_LEFT:
          left();
          break;
        case USBK_RIGHT:
          right();
          break;
        case USBK_1:
          type('1');
          break;
        case USBK_2:
          type('2');
          break;
        case USBK_3:
          type('3');
          break;
        case USBK_4:
          type('4');
          break;
        case USBK_5:
          type('5');
          break;
        case USBK_6:
          type('6');
          break;
        case USBK_7:
          type('7');
          break;
        case USBK_8:
          type('8');
          break;
        case USBK_9:
          type('9');
          break;
        case USBK_0:
          type('0');
          break;
        case USBK_A:
          type('A');
          break;
        case USBK_B:
          type('B');
          break;
        case USBK_C:
          type('C');
          break;
        case USBK_D:
          type('D');
          break;
        case USBK_E:
          type('E');
          break;
        case USBK_F:
          type('F');
          break;
      }
    }
  }
}

void HostidView::open(HostidV2::Value *hostidInOut) {
  if (isOpen)
    return;
  isOpen = true;
  memcpy(newHostid.value, hostidInOut->value, 6);
  this->hostidOut = hostidInOut;
  cursorIndex = 0;
  View::push(&HOSTID_VIEW);
}

void HostidView::ok() {
  memcpy(hostidOut->value, newHostid.value, 6);
  cancel();
}

void HostidView::cancel() {
  if (!isOpen)
    return;
  View::pop();
  isOpen = false;
}

void HostidView::left() {
  cursorIndex = cursorIndex > 0 ? cursorIndex - 1 : 5;
}

void HostidView::right() {
  cursorIndex = cursorIndex < 5 ? cursorIndex + 1 : 0;
}

void HostidView::type(unsigned char digit) {
  newHostid.value[cursorIndex] = digit;
  right();
}
