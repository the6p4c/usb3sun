#ifndef USB3SUN_HAL_H
#define USB3SUN_HAL_H

#include <cstdbool>
#include <cstddef>
#include <cstdint>

extern "C" {

#ifdef USB3SUN_HAL_ARDUINO_PICO

#include <pico/mutex.h>
typedef mutex_t usb3sun_mutex;
#define USB3SUN_MUTEX __attribute__((section(".mutex_array")))

#endif

int usb3sun_debug_read(void);
bool usb3sun_debug_write(const char *data, size_t len);
void usb3sun_allow_debug_over_cdc(void);
void usb3sun_allow_debug_over_uart(void);

bool usb3sun_fs_init(void);
// returns true iff the read succeeded, otherwise data is undefined.
bool usb3sun_fs_read(const char *path, char *data, size_t len);
bool usb3sun_fs_write(const char *path, const char *data, size_t len);

void usb3sun_mutex_lock(usb3sun_mutex *mutex);
void usb3sun_mutex_unlock(usb3sun_mutex *mutex);

uint64_t usb3sun_micros(void);
void usb3sun_sleep_micros(uint64_t micros);

bool usb3sun_gpio_read(uint8_t pin);
void usb3sun_gpio_write(uint8_t pin, bool value);
void usb3sun_gpio_set_as_inverted(uint8_t pin);
void usb3sun_gpio_set_as_output(uint8_t pin);
void usb3sun_gpio_set_as_input_pullup(uint8_t pin);
void usb3sun_gpio_set_as_input_pulldown(uint8_t pin);

void usb3sun_i2c_set_pinout(uint8_t scl, uint8_t sda);

void usb3sun_buzzer_start(uint32_t pitch);
void usb3sun_buzzer_stop(void);

void usb3sun_display_init(void);
void usb3sun_display_flush(void);
void usb3sun_display_clear(void);
void usb3sun_display_rect(
    int16_t x, int16_t y, int16_t w, int16_t h,
    int16_t border_radius, bool inverted, bool filled);
void usb3sun_display_text(int16_t x, int16_t y, bool inverted, const char *text);

}

#endif
