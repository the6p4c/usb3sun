#include "config.h"

#include <atomic>

#ifdef USB3SUN_HAL_ARDUINO_PICO
#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#endif

#include "bindings.h"
#include "buzzer.h"
#include "cli.h"
#include "hal.h"
#include "menu.h"
#include "pinout.h"
#include "settings.h"
#include "state.h"
#include "sunm.h"
#include "sunk.h"
#include "usb.h"
#include "view.h"
#include "splash.xbm"
#include "logo.xbm"

const char *const MODIFIER_NAMES[] = {
  "CtrlL", "ShiftL", "AltL", "GuiL",
  "CtrlR", "ShiftR", "AltR", "GuiR",
};

const char *const BUTTON_NAMES[] = {
  "LeftClick", "RightClick", "MiddleClick", "MouseBack", "MouseForward",
};

enum class Message : uint32_t {
  UHID_LED_FROM_STATE,
  UHID_LED_ALL_OFF,
  UHID_LED_ALL_ON,
};

// core 1 only
struct {
  bool present = false;
  uint8_t dev_addr;
  uint8_t instance;
  uint8_t if_protocol;
  struct {
    bool present = false;
    uint8_t report_id;
    uint8_t report;
  } led;
} hid[16];

std::atomic<bool> wait = true;

Pinout pinout;
State state;
Buzzer buzzer;
Settings settings;
USB3SUN_MUTEX usb3sun_mutex buzzerMutex;
USB3SUN_MUTEX usb3sun_mutex settingsMutex;

void drawStatus(int16_t x, int16_t y, const char *label, bool on);

struct DefaultView : View {
  void handlePaint() override {
    drawStatus(78, 0, "CLK", state.clickEnabled);
    drawStatus(104, 0, "BEL", state.bell);
    drawStatus(0, 18, "CAP", state.caps);
    drawStatus(26, 18, "CMP", state.compose);
    drawStatus(52, 18, "SCR", state.scroll);
    drawStatus(78, 18, "NUM", state.num);
    if (buzzer.current != Buzzer::_::NONE) {
      const auto x = 106;
      const auto y = 16;
      usb3sun_display_rect(x + 6, y + 1, 2, 11, 0, false, true);
      usb3sun_display_rect(x + 5, y + 2, 4, 10, 0, false, true);
      usb3sun_display_rect(x + 4, y + 4, 6, 8, 0, false, true);
      usb3sun_display_rect(x + 2, y + 9, 10, 3, 0, false, true);
      usb3sun_display_rect(x + 1, y + 10, 12, 2, 0, false, true);
      usb3sun_display_rect(x + 5, y + 13, 4, 1, 0, false, true);
    } else {
      usb3sun_display_rect(106, 16, 14, 14, 0, true, true);
    }
  }

  void handleKey(const UsbkChanges &changes) override {
    unsigned long t = usb3sun_micros();

#ifdef SUNK_ENABLE
    for (int i = 0; i < changes.dvLen; i++) {
      // for DV bindings, make when key makes and break when key breaks
      if (uint8_t sunkMake = USBK_TO_SUNK.dv[changes.dv[i].usbkModifier])
        sunkSend(changes.dv[i].make, sunkMake);
    }
#endif

    // treat simultaneous DV and Sel changes as DV before Sel, for special bindings
    const uint8_t lastModifiers = changes.kreport.modifier;

    for (int i = 0; i < changes.selLen; i++) {
      const uint8_t usbkSelector = changes.sel[i].usbkSelector;
      const uint8_t make = changes.sel[i].make;

      // CtrlR+Space acts like a special binding, but opens the settings menu
      // note: no other modifiers are allowed, to avoid getting them stuck down
      if (changes.sel[i].usbkSelector == USBK_SPACE && lastModifiers == USBK_CTRL_R) {
        MENU_VIEW.open();
        continue;
      }

      static bool specialBindingIsPressed[256]{};
      bool consumedBySpecialBinding[256]{};

      // for special bindings (CtrlR+Sel):
      // • make when the Sel key makes and the DV keys include CtrlR
      // • break when the Sel key breaks, even if the DV keys no longer include CtrlR
      // • do not make when CtrlR makes after the Sel key makes
      if (uint8_t sunkMake = USBK_TO_SUNK.special[usbkSelector]) {
        if (make && !!(state.lastModifiers & USBK_CTRL_R)) {
          sunkSend(true, sunkMake);
          specialBindingIsPressed[usbkSelector] = true;
          consumedBySpecialBinding[usbkSelector] = true;
        } else if (!make && specialBindingIsPressed[usbkSelector]) {
          sunkSend(false, sunkMake);
          specialBindingIsPressed[usbkSelector] = false;
          consumedBySpecialBinding[usbkSelector] = true;
        }
      }

      // for Sel bindings
      // • make when key makes and break when key breaks
      // • do not make or break when key was consumed by the corresponding special binding
      if (uint8_t sunkMake = USBK_TO_SUNK.sel[usbkSelector])
        if (!consumedBySpecialBinding[usbkSelector])
          sunkSend(make, sunkMake);
    }

#ifdef DEBUG_TIMINGS
    Sprintf("sent in %lu\n", usb3sun_micros() - t);
#endif
  }
};

static DefaultView DEFAULT_VIEW{};

void setup() {
  // pico led on, then configure pin modes
  pinout.begin();
  Sprintln("usb3sun " USB3SUN_VERSION);
  Sprintf("pinout: v%d\n", usb3sun_pinout_version());

  usb3sun_display_init();
  Settings::begin();
  settings.readAll();
  pinout.beginSun();

  View::push(&DEFAULT_VIEW);

#ifdef WAIT_PIN
  while (usb3sun_gpio_read(WAIT_PIN));
#endif
#ifdef WAIT_SERIAL
  while (Serial.read() == -1);
#endif
  wait = false;

  usb3sun_gpio_write(LED_PIN, false);
}

void drawStatus(int16_t x, int16_t y, const char *label, bool on) {
  usb3sun_display_rect(x, y, 24, 14, 4, false, on);
  usb3sun_display_text(x + 3, y + 4, on, label);
}

void loop() {
  const auto t = usb3sun_micros();
  usb3sun_display_clear();
  // display.drawXBitmap(0, 0, logo_bits, 64, 16, SSD1306_WHITE);
  usb3sun_display_text(0, 0, false, USB3SUN_VERSION);
  // static int i = 0;
  // display.printf("#%d @%lu", i++, t / 1'000);
  // display.printf("usb3sun%c", t / 500'000 % 2 == 1 ? '.' : ' ');
  View::paint();
  usb3sun_display_flush();

#ifdef UHID_LED_TEST
  static int z = 0;
  Message message = ++z % 2 == 0
    ? Message::UHID_LED_ALL_OFF
    : Message::UHID_LED_ALL_ON;
  usb3sun_fifo_push((uint32_t)message);
#endif

  int input;
  while ((input = usb3sun_debug_read()) != -1) {
    handleCliInput(input);
  }

  usb3sun_sleep_micros(10'000);
}

#ifdef SUNK_ENABLE
void sunkEvent() {
  int result;
  while ((result = usb3sun_sunk_read()) != -1) {
    uint8_t command = result;
    Sprintf("sunk: rx %02Xh\n", command);
    switch (command) {
      case SUNK_RESET: {
        // self test fail:
        // usb3sun_sunk_write(0x7E);
        // usb3sun_sunk_write(0x01);
        uint8_t response[]{SUNK_RESET_RESPONSE, 0x04, 0x7F}; // TODO optional make code
        usb3sun_sunk_write(response, sizeof response);
      } break;
      case SUNK_BELL_ON:
        state.bell = true;
        buzzer.update();
        break;
      case SUNK_BELL_OFF:
        state.bell = false;
        buzzer.update();
        break;
      case SUNK_CLICK_ON:
        state.clickEnabled = true;
        break;
      case SUNK_CLICK_OFF:
        state.clickEnabled = false;
        break;
      case SUNK_LED: {
        while ((result = usb3sun_sunk_read()) == -1) usb3sun_sleep_micros(1'000);
        uint8_t status = result;
        Sprintf("sunk: led status %02Xh\n", status);
        state.num = status & 1 << 0;
        state.compose = status & 1 << 1;
        state.scroll = status & 1 << 2;
        state.caps = status & 1 << 3;

        // ensure state update finished, then notify
        usb3sun_dmb();
        usb3sun_fifo_push((uint32_t)Message::UHID_LED_FROM_STATE);
      } break;
      case SUNK_LAYOUT: {
        // UNITED STATES (TODO alternate layouts)
        uint8_t response[]{SUNK_LAYOUT_RESPONSE, 0b00000000};
        usb3sun_sunk_write(response, sizeof response);
      } break;
    }
  }
}

void serialEvent1() {
#if defined(SUNK_ENABLE)
  sunkEvent();
#endif
}

void serialEvent2() {
#if defined(SUNK_ENABLE)
  sunkEvent();
#endif
}
#endif

void setup1() {
  while (wait);

  // Check for CPU frequency, must be multiple of 120Mhz for bit-banging USB
  uint32_t cpu_hz = usb3sun_clock_speed();
  if (cpu_hz != 120000000uL && cpu_hz != 240000000uL) {
    Sprintf("error: cpu frequency %u, set [env:pico] board_build.f_cpu = 120000000L\n", cpu_hz);
    while (true) usb3sun_sleep_micros(1'000);
  }

  usb3sun_usb_init();
}

void loop1() {
  uint32_t message;
  if (usb3sun_fifo_pop(&message)) {
    for (size_t i = 0; i < sizeof(hid) / sizeof(*hid); i++) {
      if (!hid[i].present || !hid[i].led.present)
        continue;
      uint8_t dev_addr = hid[i].dev_addr;
      uint8_t instance = hid[i].instance;
      uint8_t report_id = hid[i].led.report_id;
      switch (message) {
        case (uint32_t)Message::UHID_LED_FROM_STATE:
          hid[i].led.report =
            state.num << 0
            | state.caps << 1
            | state.scroll << 2
            | state.compose << 3;
          break;
        case (uint32_t)Message::UHID_LED_ALL_OFF:
          hid[i].led.report = 0x00;
          break;
        case (uint32_t)Message::UHID_LED_ALL_ON:
          hid[i].led.report = 0xFF;
          break;
      }
#if defined(UHID_LED_ENABLE)
#ifndef UHID_VERBOSE
      Sprint("*");
#endif
#ifdef UHID_VERBOSE
      Sprintf("hid [%zu]: usb [%u:%u]: set led report %02Xh\n", i, dev_addr, instance, hid[i].led.report);
#endif
      tuh_hid_set_report(dev_addr, instance, report_id, HID_REPORT_TYPE_OUTPUT, &hid[i].led.report, sizeof(hid[i].led.report));
#endif
    }
  }
  usb3sun_usb_task();
  buzzer.update();
}

// Invoked when device with hid interface is mounted
// Report descriptor is also available for use.
// tuh_hid_parse_report_descriptor() can be used to parse common/simple enough
// descriptor. Note: if report descriptor length > CFG_TUH_ENUMERATION_BUFSIZE,
// it will be skipped therefore report_desc = NULL, desc_len = 0
void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const *desc_report, uint16_t desc_len) {
  uint16_t vid, pid;
  usb3sun_usb_vid_pid(dev_addr, &vid, &pid);
  Sprintf("usb [%u:%u]: hid mount vid:pid=%04x:%04x\n", dev_addr, instance, vid, pid);

  usb3sun_hid_report_info reports[16];
  size_t reports_len = usb3sun_uhid_parse_report_descriptor(reports, sizeof(reports) / sizeof(*reports), desc_report, desc_len);
  for (size_t i = 0; i < reports_len; i++)
    Sprintf("    reports[%zu] report_id=%u usage=%02Xh usage_page=%04Xh\n", i, reports[i].report_id, reports[i].usage, reports[i].usage_page);

  // hid_subclass_enum_t if_subclass = ...;
  uint8_t if_protocol = usb3sun_uhid_interface_protocol(dev_addr, instance);
  Sprintf("    bInterfaceProtocol=%u", if_protocol);
  switch (if_protocol) {
    case USB3SUN_UHID_KEYBOARD:
      Sprintln(" (boot keyboard)");
      break;
    case USB3SUN_UHID_MOUSE:
      Sprintln(" (boot mouse)");
      break;
    default:
      Sprintln();
  }

  // TODO non-boot input devices
  switch (if_protocol) {
    case USB3SUN_UHID_KEYBOARD:
    case USB3SUN_UHID_MOUSE: {
      bool ok = false;
      for (size_t i = 0; i < sizeof(hid) / sizeof(*hid); i++) {
        if (!hid[i].present) {
          Sprintf(
            "hid [%zu]: usb [%u:%u], bInterfaceProtocol=%u\n",
            i, dev_addr, instance, if_protocol
          );
          hid[i].dev_addr = dev_addr;
          hid[i].instance = instance;
          hid[i].if_protocol = if_protocol;
          hid[i].led.present = false;
          if (if_protocol == USB3SUN_UHID_KEYBOARD) {
            for (size_t j = 0; j < reports_len; j++) {
              if (reports[j].usage_page == 1 && reports[j].usage == 6) {
                hid[i].led.present = true;
                hid[i].led.report_id = reports[j].report_id;
                Sprintf("hid [%zu]: led report_id=%u\n", i, hid[i].led.report_id);
              }
            }
          }
          hid[i].present = true;
          ok = true;
          break;
        }
      }
      if (!ok)
        Sprintln("error: usb [%u:%u]: hid table full");
    }
  }

  if (!usb3sun_uhid_request_report(dev_addr, instance))
    Sprintf("error: usb [%u:%u]: failed to request to receive report\n", dev_addr, instance);

  buzzer.plug();
}

// FIXME this never seems to get called?
void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance) {
  Sprintf("usb [%u:%u]: hid unmount\n", dev_addr, instance);
}

void tuh_mount_cb(uint8_t dev_addr) {
  Sprintf("usb [%u]: mount\n", dev_addr);
}

void tuh_umount_cb(uint8_t dev_addr) {
  Sprintf("usb [%u]: unmount\n", dev_addr);
  for (size_t i = 0; i < sizeof(hid) / sizeof(*hid); i++) {
    if (hid[i].present && hid[i].dev_addr == dev_addr) {
      Sprintf("hid [%zu]: removing\n", i);
      hid[i].present = false;
    }
  }
  buzzer.unplug();
}

void tuh_hid_set_protocol_complete_cb(uint8_t dev_addr, uint8_t instance, uint8_t protocol) {
  // haven’t seen this actually get printed so far, but only tried a few devices
  Sprintf("usb [%u:%u]: hid set protocol returned %u\n", dev_addr, instance, protocol);
}

// Invoked when received report from device via interrupt endpoint
void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const *report, uint16_t len) {
  uint8_t if_protocol = usb3sun_uhid_interface_protocol(dev_addr, instance);
#ifndef UHID_VERBOSE
  Sprint(".");
#endif
#ifdef UHID_VERBOSE
  Sprintf("usb [%u:%u]: hid report if_protocol=%u", dev_addr, instance, if_protocol);
#endif
#ifdef UHID_VERBOSE
  for (uint16_t i = 0; i < len; i++)
    Sprintf(" %02Xh", report[i]);
#endif

  switch (if_protocol) {
    case USB3SUN_UHID_KEYBOARD: {
      const UsbkReport *kreport = reinterpret_cast<const UsbkReport *>(report);

      unsigned long t = usb3sun_micros();

      for (int i = 0; i < 6; i++) {
        if (kreport->keycode[i] != USBK_RESERVED && kreport->keycode[i] < USBK_FIRST_KEYCODE) {
#ifdef UHID_VERBOSE
          Sprintf(" !%u\n", kreport->keycode[i]);
#endif
          goto out;
        }
      }

      UsbkChanges changes{};
      changes.kreport = *kreport;

      for (int i = 0; i < 8; i++) {
        if ((state.lastModifiers & 1 << i) != (kreport->modifier & 1 << i)) {
#ifdef UHID_VERBOSE
          Sprintf(" %c%s", kreport->modifier & 1 << i ? '+' : '-', MODIFIER_NAMES[i]);
#endif
          changes.dv[changes.dvLen++] = {(uint8_t) (1u << i), kreport->modifier & 1 << i ? true : false};
        }
      }

      for (int i = 0; i < 6; i++) {
        bool oldInNews = false;
        bool newInOlds = false;
        for (int j = 0; j < 6; j++) {
          if (state.lastKeys[i] == kreport->keycode[j])
            oldInNews = true;
          if (kreport->keycode[i] == state.lastKeys[j])
            newInOlds = true;
        }
        if (!oldInNews && state.lastKeys[i] >= USBK_FIRST_KEYCODE) {
#ifdef UHID_VERBOSE
          Sprintf(" -%u", state.lastKeys[i]);
#endif
          changes.sel[changes.selLen++] = {state.lastKeys[i], false};
        }
        if (!newInOlds && kreport->keycode[i] >= USBK_FIRST_KEYCODE) {
#ifdef UHID_VERBOSE
          Sprintf(" +%u", kreport->keycode[i]);
#endif
          changes.sel[changes.selLen++] = {kreport->keycode[i], true};
        }
      }

#ifdef UHID_VERBOSE
      Sprintln();
#endif
#ifdef DEBUG_TIMINGS
      Sprintf("diffed in %lu\n", usb3sun_micros() - t);
#endif

      View::key(changes);

      // commit the DV and Sel changes
      state.lastModifiers = changes.kreport.modifier;
      for (int i = 0; i < 6; i++)
        state.lastKeys[i] = changes.kreport.keycode[i];
    } break;
    case USB3SUN_UHID_MOUSE: {
      const UsbmReport *mreport = reinterpret_cast<const UsbmReport *>(report);
#ifdef UHID_VERBOSE
      Sprintf(" buttons=%u x=%d y=%d", mreport->buttons, mreport->x, mreport->y);
#endif

#ifdef UHID_VERBOSE
      for (int i = 0; i < 3; i++)
        if ((state.lastButtons & 1 << i) != (mreport->buttons & 1 << i))
          Sprintf(" %c%s", mreport->buttons & 1 << i ? '+' : '-', BUTTON_NAMES[i]);
      Sprintln();
#endif

      sunmSend(
        mreport->x, mreport->y,
        !!(mreport->buttons & USBM_LEFT),
        !!(mreport->buttons & USBM_MIDDLE),
        !!(mreport->buttons & USBM_RIGHT));

      state.lastButtons = mreport->buttons;
    } break;
  }
out:
  // continue to request to receive report
  if (!usb3sun_uhid_request_report(dev_addr, instance))
    Sprintf("error: usb [%u:%u]: failed to request to receive report\n", dev_addr, instance);
}

#ifdef USB3SUN_HAL_TEST

int main() {
  setup();
}

#endif
