#include "config.h"
#include "hal.h"

#include <cstdint>

#ifdef USB3SUN_HAL_ARDUINO_PICO

#define DEBUG_UART          Serial1     // UART0
#define SUNK_UART_V1        Serial1     // UART0
#define SUNK_UART_V2        Serial2     // UART1
#define SUNM_UART_V1        Serial2     // UART1

#include <pico/mutex.h>
#include <pico/platform.h>
#include <pico/time.h>
#include <hardware/clocks.h>
#include <hardware/gpio.h>
#include <Arduino.h>
#include <LittleFS.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_TinyUSB.h>

extern "C" {
#include <pio_usb.h>
}

static Adafruit_SSD1306 display{128, 32, &Wire, /* OLED_RESET */ -1};
static Adafruit_USBH_Host USBHost;
static struct {
  size_t version = 1;
  HardwareSerial *sunk = &SUNK_UART_V1;
  HardwareSerial *sunm = &SUNM_UART_V1;
  Adafruit_USBD_CDC *debugCdc = nullptr;
  SerialUART *debugUart = nullptr;
  usb3sun_pin sunkTx = SUN_KTX_V1;
  usb3sun_pin sunkRx = SUN_KRX_V1;
  usb3sun_pin sunmTx = SUN_MTX_V1;
  SerialUART *sunkUart = &SUNK_UART_V1;
  SerialUART &sunmV1 = SUNM_UART_V1;
  SerialPIO sunmV2{SUN_MTX_V2, SerialPIO::NOPIN};
} pinout;

size_t usb3sun_pinout_version(void) {
  return pinout.version;
}

void usb3sun_pinout_v2(void) {
  pinout.version = 2;
  pinout.sunk = &SUNK_UART_V2;
  pinout.sunm = &pinout.sunmV2;
  pinout.sunkTx = SUN_KTX_V2;
  pinout.sunkRx = SUN_KRX_V2;
  pinout.sunmTx = SUN_MTX_V2;
  pinout.sunkUart = &SUNK_UART_V2;
}

void usb3sun_sunk_init(void) {
  pinout.sunk->end();
  pinout.sunkUart->setPinout(pinout.sunkTx, pinout.sunkRx);
  pinout.sunk->begin(1200, SERIAL_8N1);
  // gpio invert must be set *after* setPinout/begin
  usb3sun_gpio_set_as_inverted(pinout.sunkTx);
  usb3sun_gpio_set_as_inverted(pinout.sunkRx);
}

int usb3sun_sunk_read(void) {
  return pinout.sunk->read();
}

size_t usb3sun_sunk_write(uint8_t *data, size_t len) {
  return pinout.sunk->write(data, len);
}

void usb3sun_sunm_init(uint32_t baud) {
  pinout.sunm->end();
  switch (pinout.version) {
    case 1:
      pinout.sunmV1.setPinout(SUN_MTX_V1, SUN_MRX_V1);
      break;
    case 2:
      // do nothing
      break;
  }
  pinout.sunm->begin(baud, SERIAL_8N1);
  // gpio invert must be set *after* setPinout/begin
  usb3sun_gpio_set_as_inverted(pinout.sunmTx);
}

size_t usb3sun_sunm_write(uint8_t *data, size_t len) {
  return pinout.sunm->write(data, len);
}

void usb3sun_usb_init(void) {
  pio_usb_configuration_t pio_cfg = PIO_USB_DEFAULT_CONFIG;
  pio_cfg.pin_dp = USB0_DP;
  pio_cfg.sm_tx = 1;

  // tuh_configure -> pico pio hcd_configure -> memcpy to static global
  USBHost.configure_pio_usb(1, &pio_cfg);

  // run host stack on controller (rhport) 1
  // Note: For rp2040 pico-pio-usb, calling USBHost.begin() on core1 will have most of the
  // host bit-banging processing works done in core1 to free up core0 for other works
  // tuh_init -> pico pio hcd_init -> pio_usb_host_init -> pio_usb_bus_init -> set root[0]->initialized
  USBHost.begin(1);

  // set root[i]->initialized for the first unused i less than PIO_USB_ROOT_PORT_CNT
  pio_usb_host_add_port(USB1_DP, PIO_USB_PINOUT_DPDM);
}

void usb3sun_usb_task(void) {
  USBHost.task();
}

bool usb3sun_usb_vid_pid(uint8_t dev_addr, uint16_t *vid, uint16_t *pid) {
  return tuh_vid_pid_get(dev_addr, vid, pid);
}

bool usb3sun_uhid_request_report(uint8_t dev_addr, uint8_t instance) {
  return tuh_hid_receive_report(dev_addr, instance);
}

uint8_t usb3sun_uhid_interface_protocol(uint8_t dev_addr, uint8_t instance) {
  return tuh_hid_interface_protocol(dev_addr, instance);
}

size_t usb3sun_uhid_parse_report_descriptor(usb3sun_hid_report_info *result, size_t result_len, const uint8_t *descriptor, size_t descriptor_len) {
  tuh_hid_report_info_t tuh_result[16];
  size_t tuh_result_len = tuh_hid_parse_report_descriptor(
    tuh_result, sizeof tuh_result / sizeof *tuh_result,
    descriptor, descriptor_len);
  for (size_t i = 0; i < tuh_result_len && i < result_len; i++) {
    result[i].report_id = tuh_result[i].report_id;
    result[i].usage = tuh_result[i].usage;
    result[i].usage_page = tuh_result[i].usage_page;
  }
  return tuh_result_len;
  hid_mouse_report_t x;
}

void usb3sun_debug_init(int (*printf)(const char *format, ...)) {
#if defined(DEBUG_LOGGING)
  DEBUG_RP2040_PRINTF = printf;
#endif
}

int usb3sun_debug_read(void) {
  return pinout.debugUart ? pinout.debugUart->read() : -1;
}

bool usb3sun_debug_write(const char *data, size_t len) {
  bool ok = true;
  if (pinout.debugCdc) {
    if (pinout.debugCdc->write(data, len) < len) {
      ok = false;
    } else {
      pinout.debugCdc->flush();
    }
  }
  if (pinout.debugUart) {
    if (pinout.debugUart->write(data, len) < len) {
      ok = false;
    } else {
      pinout.debugUart->flush();
    }
  }
  return ok;
}

void usb3sun_allow_debug_over_cdc(void) {
  // needs to be done manually when using FreeRTOS and/or TinyUSB
  Serial.begin(115200);
  pinout.debugCdc = &Serial;
}

void usb3sun_allow_debug_over_uart(void) {
  DEBUG_UART.end();
  DEBUG_UART.setPinout(DEBUG_UART_TX, DEBUG_UART_RX);
  DEBUG_UART.setFIFOSize(4096);
  DEBUG_UART.begin(DEBUG_UART_BAUD, SERIAL_8N1);
#if CFG_TUSB_DEBUG
  TinyUSB_Serial_Debug = &DEBUG_UART;
#endif
  pinout.debugUart = &DEBUG_UART;
}

bool usb3sun_fs_init(void) {
  LittleFSConfig cfg;
  cfg.setAutoFormat(true);
  LittleFS.setConfig(cfg);
  return LittleFS.begin();
}

bool usb3sun_fs_read(const char *path, char *data, size_t len) {
  if (File f = LittleFS.open(path, "r")) {
    size_t result = f.readBytes(data, len);
    f.close();
    return result == len;
  }
  return false;
}

bool usb3sun_fs_write(const char *path, const char *data, size_t len) {
  if (File f = LittleFS.open(path, "w")) {
    size_t result = f.write(data, len);
    f.close();
    return result == len;
  }
  return false;
}

void usb3sun_mutex_lock(usb3sun_mutex *mutex) {
  mutex_enter_blocking(mutex);
}

void usb3sun_mutex_unlock(usb3sun_mutex *mutex) {
  mutex_exit(mutex);
}

bool usb3sun_fifo_push(uint32_t value) {
  return rp2040.fifo.push_nb(value);
}

bool usb3sun_fifo_pop(uint32_t *result) {
  return rp2040.fifo.pop_nb(result);
}

uint64_t usb3sun_micros(void) {
  return micros();
}

void usb3sun_sleep_micros(uint64_t micros) {
  sleep_us(micros);
}

uint32_t usb3sun_clock_speed(void) {
  return clock_get_hz(clk_sys);
}

void usb3sun_panic(const char *format, ...) {
  // TODO handle format args
  panic("%s", format);
}

static int64_t alarm(alarm_id_t, void *callback) {
  reinterpret_cast<void (*)(void)>(callback)();
  return 0; // don’t reschedule
}

void usb3sun_alarm(uint32_t ms, void (*callback)(void)) {
  add_alarm_in_ms(ms, alarm, reinterpret_cast<void *>(callback), true);
}

bool usb3sun_gpio_read(uint8_t pin) {
  return gpio_get(pin);
}

void usb3sun_gpio_write(uint8_t pin, bool value) {
  digitalWrite(pin, value ? HIGH : LOW);
}

void usb3sun_gpio_set_as_inverted(uint8_t pin) {
  gpio_set_outover(pin, GPIO_OVERRIDE_INVERT);
  gpio_set_inover(pin, GPIO_OVERRIDE_INVERT);
}

void usb3sun_gpio_set_as_output(uint8_t pin) {
  pinMode(pin, OUTPUT);
}

void usb3sun_gpio_set_as_input_pullup(uint8_t pin) {
  pinMode(pin, INPUT_PULLUP);
}

void usb3sun_gpio_set_as_input_pulldown(uint8_t pin) {
  pinMode(pin, INPUT_PULLDOWN);
}

void usb3sun_i2c_set_pinout(uint8_t scl, uint8_t sda) {
  Wire.setSCL(scl);
  Wire.setSDA(sda);
}

void usb3sun_buzzer_start(uint32_t pitch) {
  analogWriteRange(100);
  analogWriteFreq(pitch);
  analogWrite(BUZZER_PIN, 50);
}

void usb3sun_buzzer_stop(void) {
  digitalWrite(BUZZER_PIN, false);
}

void usb3sun_display_init(void) {
  display.begin(SSD1306_SWITCHCAPVCC, /* SCREEN_ADDRESS */ 0x3C);
  display.setRotation(DISPLAY_ROTATION);
  display.cp437(true);
  display.setTextWrap(false);
  display.clearDisplay();
  // display.drawXBitmap(0, 0, splash_bits, 128, 32, SSD1306_WHITE);
  display.display();
}

void usb3sun_display_flush(void) {
  display.display();
}

void usb3sun_display_clear(void) {
  display.clearDisplay();
}

void usb3sun_display_rect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t border_radius, bool inverted, bool filled) {
  if (filled) {
    display.fillRoundRect(x, y, w, h, border_radius, inverted ? SSD1306_BLACK : SSD1306_WHITE);
  } else {
    display.drawRoundRect(x, y, w, h, border_radius, inverted ? SSD1306_BLACK : SSD1306_WHITE);
  }
}

void usb3sun_display_text(int16_t x, int16_t y, bool inverted, const char *text) {
  display.setTextColor(
    inverted ? SSD1306_BLACK : SSD1306_WHITE,
    inverted ? SSD1306_WHITE : SSD1306_BLACK);
  display.setCursor(x, y);
  display.print(text);
}

#elifdef USB3SUN_HAL_TEST

#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <ctime>

#include <unistd.h>
#include <fcntl.h>

static struct {
  size_t version = 1;
} pinout;

size_t usb3sun_pinout_version(void) {
  return pinout.version;
}

void usb3sun_pinout_v2(void) {
  pinout.version = 2;
}

void usb3sun_sunk_init(void) {}

int usb3sun_sunk_read(void) {
  return -1;
}

size_t usb3sun_sunk_write(uint8_t *data, size_t len) {
  return 0;
}

void usb3sun_sunm_init(uint32_t baud) {}

size_t usb3sun_sunm_write(uint8_t *data, size_t len) {
  return 0;
}

void usb3sun_usb_init(void) {}

void usb3sun_usb_task(void) {}

bool usb3sun_usb_vid_pid(uint8_t dev_addr, uint16_t *vid, uint16_t *pid) {
  return false;
}

bool usb3sun_uhid_request_report(uint8_t dev_addr, uint8_t instance) {
  return false;
}

uint8_t usb3sun_uhid_interface_protocol(uint8_t dev_addr, uint8_t instance) {
  return 0;
}

size_t usb3sun_uhid_parse_report_descriptor(usb3sun_hid_report_info *result, size_t result_len, const uint8_t *descriptor, size_t descriptor_len) {
  return 0;
}

void usb3sun_debug_init(int (*printf)(const char *format, ...)) {}

int usb3sun_debug_read(void) {
  if (fcntl(0, F_SETFL, O_NONBLOCK) == 0) {
    uint8_t result;
    if (read(0, &result, sizeof result) == sizeof result) {
      return result;
    }
  }
  return -1;
}

bool usb3sun_debug_write(const char *data, size_t len) {
  bool ok = fwrite(data, 1, len, stdout) == len;
  fflush(stdout);
  return ok;
}

void usb3sun_allow_debug_over_cdc(void) {}

void usb3sun_allow_debug_over_uart(void) {}

bool usb3sun_fs_init(void) {
  return false;
}

bool usb3sun_fs_read(const char *path, char *data, size_t len) {
  return false;
}

bool usb3sun_fs_write(const char *path, const char *data, size_t len) {
  return false;
}

void usb3sun_mutex_lock(usb3sun_mutex *mutex) {}

void usb3sun_mutex_unlock(usb3sun_mutex *mutex) {}

bool usb3sun_fifo_push(uint32_t value) {
  return false;
}

bool usb3sun_fifo_pop(uint32_t *result) {
  return false;
}

uint64_t usb3sun_micros(void) {
  static uint64_t result = 0;
  struct timespec ts;
  if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
    result = (uint64_t)ts.tv_sec * 1'000'000
      + (uint64_t)ts.tv_nsec / 1'000;
  }
  return result;
}

void usb3sun_sleep_micros(uint64_t micros) {
  struct timespec ts;
  if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
    ts.tv_nsec += micros * 1'000;
    while (ts.tv_nsec > 999'999'999) {
      ts.tv_sec += 1;
      ts.tv_nsec -= 1'000'000'000;
    }
    while (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts, nullptr) == EINTR);
  }
}

uint32_t usb3sun_clock_speed(void) {
  return 120'000'000;
}

void usb3sun_panic(const char *format, ...) {
  va_list ap;
  va_start(ap, format);
  vfprintf(stdout, format, ap);
  va_end(ap);
  fflush(stdout);
  abort();
}

void usb3sun_alarm(uint32_t ms, void (*callback)(void)) {}

bool usb3sun_gpio_read(usb3sun_pin pin) {
  return false;
}

void usb3sun_gpio_write(usb3sun_pin pin, bool value) {}

void usb3sun_gpio_set_as_inverted(usb3sun_pin pin) {}

void usb3sun_gpio_set_as_output(usb3sun_pin pin) {}

void usb3sun_gpio_set_as_input_pullup(usb3sun_pin pin) {}

void usb3sun_gpio_set_as_input_pulldown(usb3sun_pin pin) {}

void usb3sun_i2c_set_pinout(usb3sun_pin scl, usb3sun_pin sda) {}

void usb3sun_buzzer_start(uint32_t pitch) {}

void usb3sun_buzzer_stop(void) {}

void usb3sun_display_init(void) {}

void usb3sun_display_flush(void) {}

void usb3sun_display_clear(void) {}

void usb3sun_display_rect(
    int16_t x, int16_t y, int16_t w, int16_t h,
    int16_t border_radius, bool inverted, bool filled) {}

void usb3sun_display_text(int16_t x, int16_t y, bool inverted, const char *text) {}

#endif
