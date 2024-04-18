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

std::atomic<bool> waiting = true;

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
  waiting = false;

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
#endif

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

void setup1() {
  while (waiting);

  // Check for CPU frequency, must be multiple of 120Mhz for bit-banging USB
  uint32_t cpu_hz = usb3sun_clock_speed();
  if (cpu_hz != 120000000uL && cpu_hz != 240000000uL) {
    usb3sun_panic("error: cpu frequency %u, set [env:pico] board_build.f_cpu = 120000000L\n", cpu_hz);
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
    default: {
#ifdef UHID_VERBOSE
      Sprintln();
#endif
    } break;
  }
out:
  // continue to request to receive report
  if (!usb3sun_uhid_request_report(dev_addr, instance))
    Sprintf("error: usb [%u:%u]: failed to request to receive report\n", dev_addr, instance);
}

#ifdef USB3SUN_HAL_TEST

#include <cstring>
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>

#define TEST_REQUIRES(expr) do { fprintf(stderr, ">>> skipping test (%s)\n", #expr); return true; } while (0)
#define TEST_ASSERT_EQ(actual, expected, fmt) do { if (actual != expected) { fprintf(stderr, "\nassertion failed: %s\n    actual: " fmt "\n    expected: " fmt "\n", #actual, actual, expected); return false; } } while (0)

static bool assert_then_clear_test_history(const std::vector<Op> &expected) {
  const std::vector<Entry> &actual = usb3sun_test_get_history();
  std::optional<size_t> first_difference{};
  for (size_t i = 0; i < actual.size() || i < expected.size(); i++) {
    if ((i < actual.size()) != (i < expected.size()) || actual[i].op != expected[i]) {
      first_difference = i;
      break;
    }
  }
  if (first_difference.has_value()) {
    std::cerr << "\nassertion failed: bad test history!\n";
    for (size_t i = 0; i < *first_difference; i++) {
      std::cerr << "    " << actual[i] << "\n";
    }
    for (size_t i = *first_difference; i < actual.size() || i < expected.size(); i++) {
      std::cerr << "!!!";
      if (i < actual.size()) {
        std::cerr << " " << actual[i];
      }
      if (i < expected.size()) {
        std::cerr << " (expected " << expected[i] << ")";
      }
      std::cerr << "\n";
    }
  }
  usb3sun_test_clear_history();
  return !first_difference.has_value();
}

static std::vector<const char *> test_names = {
  "setup_pinout_v1",
  "setup_pinout_v2",
  "sunk_reset",
  "uhid_mount",
  "buzzer_bell",
  "buzzer_click",
  "settings_read_ok",
  "settings_read_not_found",
  "settings_read_wrong_version",
  "settings_read_too_short",
};

static void help() {
  std::cerr << "usage: path/to/program <demo|all|test_name>\n";
  std::cerr << "...where test_name can be one of:\n";
  for (const char *&name : test_names) {
    std::cerr << "    " << name << "\n";
  }
}

static bool run_test(const char *test_name) {
  if (!strcmp(test_name, "setup_pinout_v1")) {
    usb3sun_test_init(PinoutV2Op::id | SunkInitOp::id | SunmInitOp::id | GpioWriteOp::id | GpioReadOp::id);
    usb3sun_mock_gpio_read(PINOUT_V2_PIN, false);
    setup();
    return assert_then_clear_test_history(std::vector<Op> {
      GpioWriteOp {LED_PIN, true},
      GpioReadOp {PINOUT_V2_PIN, false},
#ifdef SUNK_ENABLE
      SunkInitOp {},
#endif
#ifdef SUNM_ENABLE
      SunmInitOp {},
#endif
      GpioWriteOp {LED_PIN, false},
    });
  }

  if (!strcmp(test_name, "setup_pinout_v2")) {
    usb3sun_test_init(PinoutV2Op::id | SunkInitOp::id | SunmInitOp::id | GpioWriteOp::id | GpioReadOp::id);
    usb3sun_mock_gpio_read(PINOUT_V2_PIN, true);
    setup();
    return assert_then_clear_test_history(std::vector<Op> {
      GpioWriteOp {LED_PIN, true},
      GpioReadOp {PINOUT_V2_PIN, true},
      PinoutV2Op {},
      GpioWriteOp {DISPLAY_ENABLE, true},
#ifdef SUNK_ENABLE
      SunkInitOp {},
      GpioWriteOp {KTX_ENABLE, false},
#endif
#ifdef SUNM_ENABLE
      SunmInitOp {},
#endif
      GpioWriteOp {LED_PIN, false},
    });
  }

  if (!strcmp(test_name, "sunk_reset")) {
#ifndef SUNK_ENABLE
    TEST_REQUIRES(SUNK_ENABLE);
#endif
    usb3sun_test_init(SunkReadOp::id | SunkWriteOp::id);
    usb3sun_mock_sunk_read("\x01", 1); // SUNK_RESET
    setup();
    while (usb3sun_mock_sunk_read_has_input()) {
      serialEvent1();
      serialEvent2();
    }
    return assert_then_clear_test_history(std::vector<Op> {
      SunkReadOp {},
      SunkWriteOp {{0xFF, 0x04, 0x7F}},
      SunkReadOp {},
      SunkReadOp {},
    });
  }

  if (!strcmp(test_name, "uhid_mount")) {
    usb3sun_test_init(UhidRequestReportOp::id);
    setup();

    uint8_t empty[]{};
    usb3sun_mock_usb_vid_pid(true, 0x045E, 0x0750); // Microsoft Wired Keyboard 600
    usb3sun_mock_uhid_parse_report_descriptor(std::vector<usb3sun_hid_report_info> {
        usb3sun_hid_report_info {0, 0x06, 0x0001},
    });
    usb3sun_mock_uhid_interface_protocol(USB3SUN_UHID_KEYBOARD);
    usb3sun_mock_uhid_request_report_result(true);
    tuh_hid_mount_cb(1, 0, empty, 0);

    usb3sun_mock_uhid_parse_report_descriptor(std::vector<usb3sun_hid_report_info> {
        usb3sun_hid_report_info {1, 0x01, 0x000C},
        usb3sun_hid_report_info {3, 0x80, 0x0001},
    });
    usb3sun_mock_uhid_interface_protocol(0);
    tuh_hid_mount_cb(1, 1, empty, 0);

    // TODO assert something else that’s true for [1:0] but not true for [1:1]
    return assert_then_clear_test_history(std::vector<Op> {
      UhidRequestReportOp {1, 0},
      UhidRequestReportOp {1, 1},
    });
  }

  if (!strcmp(test_name, "buzzer_bell")) {
#ifndef SUNK_ENABLE
    TEST_REQUIRES(SUNK_ENABLE);
#endif
    usb3sun_test_init(BuzzerStartOp::id | GpioWriteOp::id);
    usb3sun_mock_sunk_read("\x01\x02\x03", 3); // SUNK_RESET, SUNK_BELL_ON, SUNK_BELL_OFF
    setup();
    while (usb3sun_mock_sunk_read_has_input()) {
      serialEvent1();
      serialEvent2();
    }
    return assert_then_clear_test_history(std::vector<Op> {
      GpioWriteOp {LED_PIN, true},
      GpioWriteOp {DISPLAY_ENABLE, true},
      GpioWriteOp {KTX_ENABLE, false},
      GpioWriteOp {LED_PIN, false},
      BuzzerStartOp {2083},
      GpioWriteOp {BUZZER_PIN, false},
    });
  }

  if (!strcmp(test_name, "buzzer_click")) {
#ifndef SUNK_ENABLE
    TEST_REQUIRES(SUNK_ENABLE);
#endif
    const auto pumpSunkInput = []() {
      while (usb3sun_mock_sunk_read_has_input()) {
        serialEvent1();
        serialEvent2();
      }
    };
    const auto pressKey = []() {
      sunkSend(true, SUNK_RETURN);
      sunkSend(false, SUNK_RETURN);
    };
    const auto pumpBuzzerUpdates = []() {
      uint64_t t_start = usb3sun_micros();
      while (usb3sun_micros() - t_start < 5'000) {
        loop1();
      }
      loop1();
    };
    usb3sun_test_init(BuzzerStartOp::id | GpioWriteOp::id);
    usb3sun_mock_sunk_read("\x01\x02", 2); // SUNK_RESET, SUNK_BELL_ON

    // get the setup out of the way.
    setup();
    if (!assert_then_clear_test_history(std::vector<Op> {
      GpioWriteOp {LED_PIN, true},
      GpioWriteOp {DISPLAY_ENABLE, true},
      GpioWriteOp {KTX_ENABLE, false},
      GpioWriteOp {LED_PIN, false},
    })) return false;

    // no click by default.
    pumpSunkInput();
    pressKey();
    pumpBuzzerUpdates();
    if (!assert_then_clear_test_history(std::vector<Op> {
      BuzzerStartOp {2083},
    })) return false;

    // click when workstation enables click mode.
    usb3sun_mock_sunk_read("\x0A", 1); // SUNK_CLICK_ON
    pumpSunkInput();
    pressKey();
    pumpBuzzerUpdates();
    if (!assert_then_clear_test_history(std::vector<Op> {
      BuzzerStartOp {1000},
      BuzzerStartOp {2083},
    })) return false;

    // no click when workstation disables click mode.
    usb3sun_mock_sunk_read("\x0B", 1); // SUNK_CLICK_OFF
    pumpSunkInput();
    pressKey();
    pumpBuzzerUpdates();
    if (!assert_then_clear_test_history(std::vector<Op> {
    })) return false;

    // click when forceClick is on, even when click mode is disabled.
    settings.forceClick().current = ForceClick::_::ON;
    pumpSunkInput();
    pressKey();
    pumpBuzzerUpdates();
    if (!assert_then_clear_test_history(std::vector<Op> {
      BuzzerStartOp {1000},
      BuzzerStartOp {2083},
    })) return false;

    // no click when forceClick is off, even when click mode is enabled.
    settings.forceClick().current = ForceClick::_::OFF;
    usb3sun_mock_sunk_read("\x0A", 1); // SUNK_CLICK_ON
    pumpSunkInput();
    pressKey();
    pumpBuzzerUpdates();
    if (!assert_then_clear_test_history(std::vector<Op> {
    })) return false;

    return true;
  }

  if (!strcmp(test_name, "settings_read_ok")) {
    usb3sun_test_init(0);
    usb3sun_mock_fs_read([](const char *path, char *data, size_t data_len, size_t &actual_len) {
      if (!!strcmp(path, "/clickDuration")) return false;
      //            [      version ][      padding ][                unsigned long ]
      memcpy(data, "\x01\x00\x00\x00\xAA\xAA\xAA\xAA\x55\x55\x55\x55\x55\x55\x55\x55", actual_len = 16);
      return true;
    });
    setup();
    TEST_ASSERT_EQ(settings.clickDuration(), 0x5555555555555555UL, "%ju");
    return true;
  }

  if (!strcmp(test_name, "settings_read_not_found")) {
    usb3sun_test_init(0);
    usb3sun_mock_fs_read([](const char *path, char *data, size_t data_len, size_t &actual_len) {
      return false;
    });
    setup();
    TEST_ASSERT_EQ(settings.clickDuration(), 5, "%ju");
    return true;
  }

  if (!strcmp(test_name, "settings_read_wrong_version")) {
    usb3sun_test_init(0);
    usb3sun_mock_fs_read([](const char *path, char *data, size_t data_len, size_t &actual_len) {
      if (!!strcmp(path, "/clickDuration")) return false;
      //            [      version ][      padding ][                unsigned long ]
      memcpy(data, "\x00\x00\x00\x00\xAA\xAA\xAA\xAA\x55\x55\x55\x55\x55\x55\x55\x55", actual_len = 16);
      return true;
    });
    setup();
    TEST_ASSERT_EQ(settings.clickDuration(), 5, "%ju");
    return true;
  }

  if (!strcmp(test_name, "settings_read_too_short")) {
    usb3sun_test_init(0);
    usb3sun_mock_fs_read([](const char *path, char *data, size_t data_len, size_t &actual_len) {
      if (!!strcmp(path, "/clickDuration")) return false;
      //            [      version ][      padding ][                  7 bytes ]
      memcpy(data, "\x01\x00\x00\x00\xAA\xAA\xAA\xAA\x55\x55\x55\x55\x55\x55\x55", actual_len = 15);
      return true;
    });
    setup();
    TEST_ASSERT_EQ(settings.clickDuration(), 5, "%ju");
    return true;
  }

  help();
  return false;
}

int main(int argc, char **argv) {
  if (argc == 2) {
    const char *test_name = argv[1];
    if (!strcmp(test_name, "demo")) {
      usb3sun_test_init(0);
      setup();
      setup1();
      while (true) {
        loop();
        loop1();
      }
    } else if (!strcmp(test_name, "all")) {
      for (const char *&name : test_names) {
        std::cerr << ">>> starting test: " << name << "\n";
        test_name = name;
        pid_t pid = fork();
        if (pid == 0) {
          if (run_test(test_name)) {
            return 0;
          } else {
            std::cerr << ">>> test failed: " << test_name << "\n";
            return 1;
          }
        } else if (pid == -1) {
          perror("fatal: fork");
          return 1;
        } else {
          int result;
          wait(&result);
          if (!WIFEXITED(result) || WEXITSTATUS(result) != 0) {
            std::cerr << "fatal: wait(2) returned " << result << "\n";
            return 1;
          }
        }
      }
      return 0;
    } else {
      return run_test(test_name) ? 0 : 1;
    }
  }
  help();
  return 0;
}

#endif
