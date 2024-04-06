#ifndef USB3SUN_HAL_H
#define USB3SUN_HAL_H

#include <cstdbool>
#include <cstddef>
#include <cstdint>

extern "C" {

typedef uint8_t usb3sun_pin;

#ifdef USB3SUN_HAL_ARDUINO_PICO
  #include <pico/mutex.h>
  #include <hardware/sync.h>
  typedef mutex_t usb3sun_mutex;
  #define USB3SUN_MUTEX __attribute__((section(".mutex_array")))
  #define usb3sun_dmb() __dmb()
#elifdef USB3SUN_HAL_TEST
  struct usb3sun_mutex {};
  #define USB3SUN_MUTEX // empty
  #define usb3sun_dmb() do {} while (0)

  extern "C++" {
    #include <iostream>
    #include <variant>
    #include <vector>
    struct PinoutV2Op {};
    struct SunkInitOp {};
    struct SunkReadOp {};
    struct SunkWriteOp { std::vector<uint8_t> data; };
    struct SunmInitOp {};
    struct SunmWriteOp { std::vector<uint8_t> data; };
    struct GpioReadOp { usb3sun_pin pin; bool value; };
    struct GpioWriteOp { usb3sun_pin pin; bool value; };
    using Op = std::variant<
      PinoutV2Op,
      SunkInitOp,
      SunkReadOp,
      SunkWriteOp,
      SunmInitOp,
      SunmWriteOp,
      GpioReadOp,
      GpioWriteOp>;
    struct Entry {
      uint64_t micros;
      Op op;
    };
    bool operator==(const PinoutV2Op &p, const PinoutV2Op &q);
    bool operator==(const SunkInitOp &p, const SunkInitOp &q);
    bool operator==(const SunkReadOp &p, const SunkReadOp &q);
    bool operator==(const SunkWriteOp &p, const SunkWriteOp &q);
    bool operator==(const SunmInitOp &p, const SunmInitOp &q);
    bool operator==(const SunmWriteOp &p, const SunmWriteOp &q);
    bool operator==(const GpioReadOp &p, const GpioReadOp &q);
    bool operator==(const GpioWriteOp &p, const GpioWriteOp &q);
    bool operator!=(const PinoutV2Op &p, const PinoutV2Op &q);
    bool operator!=(const SunkInitOp &p, const SunkInitOp &q);
    bool operator!=(const SunkReadOp &p, const SunkReadOp &q);
    bool operator!=(const SunkWriteOp &p, const SunkWriteOp &q);
    bool operator!=(const SunmInitOp &p, const SunmInitOp &q);
    bool operator!=(const SunmWriteOp &p, const SunmWriteOp &q);
    bool operator!=(const GpioReadOp &p, const GpioReadOp &q);
    bool operator!=(const GpioWriteOp &p, const GpioWriteOp &q);
    std::ostream& operator<<(std::ostream& s, const PinoutV2Op &o);
    std::ostream& operator<<(std::ostream& s, const SunkInitOp &o);
    std::ostream& operator<<(std::ostream& s, const SunkReadOp &o);
    std::ostream& operator<<(std::ostream& s, const SunkWriteOp &o);
    std::ostream& operator<<(std::ostream& s, const SunmInitOp &o);
    std::ostream& operator<<(std::ostream& s, const SunmWriteOp &o);
    std::ostream& operator<<(std::ostream& s, const GpioReadOp &o);
    std::ostream& operator<<(std::ostream& s, const GpioWriteOp &o);
    std::ostream& operator<<(std::ostream& s, const Op &o);
    std::ostream& operator<<(std::ostream& s, const Entry& v);
    void usb3sun_test_init(void);
    void usb3sun_mock_gpio_read(usb3sun_pin pin, bool value);
    const std::vector<Entry> &usb3sun_test_get_history(void);
  }
#endif

// based on tinyusb’s tuh_hid_report_info_t
typedef struct {
  uint8_t report_id;
  uint8_t usage;
  uint16_t usage_page;
} usb3sun_hid_report_info;

size_t usb3sun_pinout_version(void);
void usb3sun_pinout_v2(void);

void usb3sun_sunk_init(void);
int usb3sun_sunk_read(void);
size_t usb3sun_sunk_write(uint8_t *data, size_t len);

void usb3sun_sunm_init(uint32_t baud);
size_t usb3sun_sunm_write(uint8_t *data, size_t len);

void usb3sun_usb_init(void);
void usb3sun_usb_task(void);
bool usb3sun_usb_vid_pid(uint8_t dev_addr, uint16_t *vid, uint16_t *pid);
bool usb3sun_uhid_request_report(uint8_t dev_addr, uint8_t instance);
uint8_t usb3sun_uhid_interface_protocol(uint8_t dev_addr, uint8_t instance);
size_t usb3sun_uhid_parse_report_descriptor(usb3sun_hid_report_info *result, size_t result_len, const uint8_t *descriptor, size_t descriptor_len);

void usb3sun_debug_init(int (*printf)(const char *format, ...));
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

bool usb3sun_fifo_push(uint32_t value);
bool usb3sun_fifo_pop(uint32_t *result);

uint64_t usb3sun_micros(void);
void usb3sun_sleep_micros(uint64_t micros);
uint32_t usb3sun_clock_speed(void);
void usb3sun_panic(const char *format, ...);
void usb3sun_alarm(uint32_t ms, void (*callback)(void));

bool usb3sun_gpio_read(usb3sun_pin pin);
void usb3sun_gpio_write(usb3sun_pin pin, bool value);
void usb3sun_gpio_set_as_inverted(usb3sun_pin pin);
void usb3sun_gpio_set_as_output(usb3sun_pin pin);
void usb3sun_gpio_set_as_input_pullup(usb3sun_pin pin);
void usb3sun_gpio_set_as_input_pulldown(usb3sun_pin pin);

void usb3sun_i2c_set_pinout(usb3sun_pin scl, usb3sun_pin sda);

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
