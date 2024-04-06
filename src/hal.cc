#include "config.h"
#include "hal.h"
#include <cstdint>

#ifdef USB3SUN_HAL_ARDUINO_PICO

#include <pico/time.h>
#include <hardware/gpio.h>
#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_TinyUSB.h>

static Adafruit_SSD1306 display{128, 32, &Wire, /* OLED_RESET */ -1};
static Adafruit_USBD_CDC *debugCdc = nullptr;
static SerialUART *debugUart = nullptr;

int usb3sun_debug_read(void) {
  return debugUart ? debugUart->read() : -1;
}

bool usb3sun_debug_write(const char *data, size_t len) {
  bool ok = true;
  if (debugCdc) {
    if (debugCdc->write(data, len) < len) {
      ok = false;
    } else {
      debugCdc->flush();
    }
  }
  if (debugUart) {
    if (debugUart->write(data, len) < len) {
      ok = false;
    } else {
      debugUart->flush();
    }
  }
  return ok;
}

void usb3sun_allow_debug_over_cdc(void) {
  // needs to be done manually when using FreeRTOS and/or TinyUSB
  Serial.begin(115200);
  debugCdc = &Serial;
}

void usb3sun_allow_debug_over_uart(void) {
  DEBUG_UART.end();
  DEBUG_UART.setPinout(DEBUG_UART_TX, DEBUG_UART_RX);
  DEBUG_UART.setFIFOSize(4096);
  DEBUG_UART.begin(DEBUG_UART_BAUD, SERIAL_8N1);
#if CFG_TUSB_DEBUG
  TinyUSB_Serial_Debug = &DEBUG_UART;
#endif
  debugUart = &DEBUG_UART;
}

uint64_t usb3sun_micros(void) {
  return micros();
}

void usb3sun_sleep_micros(uint64_t micros) {
  sleep_us(micros);
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

// void usb3sun_add_alarm(uint32_t ms, void (*callback)(void)) {
//   add_alarm_in_ms(ms, alarm_callback_t callback, void *user_data, bool fire_if_past)
// }

#endif
