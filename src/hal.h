#ifndef USB3SUN_HAL_H
#define USB3SUN_HAL_H

#include <cstdbool>
#include <cstdint>

extern "C" {

uint64_t usb3sun_micros(void);
void usb3sun_sleep_micros(uint64_t micros);

bool usb3sun_gpio_read(uint8_t pin);
void usb3sun_gpio_write(uint8_t pin, bool value);
void usb3sun_gpio_set_as_inverted(uint8_t pin);
void usb3sun_gpio_set_as_output(uint8_t pin);
void usb3sun_gpio_set_as_input_pullup(uint8_t pin);
void usb3sun_gpio_set_as_input_pulldown(uint8_t pin);

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
