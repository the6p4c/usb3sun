#ifndef USB3SUN_SETTINGS_H
#define USB3SUN_SETTINGS_H

#include <algorithm>
#include <cstdint>
#include <cstring>

#include "hal.h"
#include "mutex.h"
#include "pinout.h"

#define SETTING_V1_WRAPPER_TYPE(_wrapper_name, _file_name, _padding_before, _payload_type, _padding_after, ...) \
  struct __attribute__((packed)) _wrapper_name { \
    static const uint32_t currentVersion = 1; \
    static constexpr const char *const path = "/" _file_name; \
    uint32_t version = currentVersion; \
    uint8_t paddingBefore[_padding_before]; \
    _payload_type value = __VA_ARGS__; \
    uint8_t paddingAfter[_padding_after]; \
    inline bool operator==(const _wrapper_name &other) const { \
      return this->value == other.value; \
    } \
  };

#define SETTING_ENUM(_name, ...) \
struct _name { \
  typedef enum class State: int32_t { __VA_ARGS__, VALUE_COUNT } _; \
  State current; \
  operator State&() { return current; } \
  inline bool operator==(const _name &other) const { \
    return this->current == other.current; \
  } \
  State& operator++() { \
    current = static_cast<State>(std::max(0, std::min(static_cast<int>(_::VALUE_COUNT) - 1, static_cast<int>(current) + 1))); \
    return current; \
  } \
  State& operator--() { \
    current = static_cast<State>(std::max(0, std::min(static_cast<int>(_::VALUE_COUNT) - 1, static_cast<int>(current) - 1))); \
    return current; \
  } \
};

extern usb3sun_mutex settingsMutex;

SETTING_ENUM(ForceClick, NO, OFF, ON);
SETTING_ENUM(MouseBaud, S1200, S2400, S4800, S9600);
struct ClickDurationV2 {
  static constexpr const char *const path = "/clickDuration.v2";
  using Value = uint64_t;
  static constexpr Value defaultValue {5}; // [0,100]
};
struct ForceClickV2 {
  static constexpr const char *const path = "/forceClick.v2";
  using Value = ForceClick;
  static constexpr Value defaultValue {ForceClick::_::NO};
};
struct MouseBaudV2 {
  static constexpr const char *const path = "/mouseBaud.v2";
  using Value = MouseBaud;
  static constexpr Value defaultValue {MouseBaud::_::S9600};
};
struct HostidV2 {
  static constexpr const char *const path = "/hostid.v2";
  // wrapper type to ensure that hostid values are modifiable lvalues.
  struct Value {
    uint8_t value[6];
    const uint8_t &operator[](size_t i) const { return value[i]; }
    uint8_t &operator[](size_t i) { return value[i]; }
    inline bool operator==(const Value &other) const {
      return !memcmp(value, other.value, sizeof value);
    }
    inline bool operator!=(const Value &other) const {
      return !(*this == other);
    }
  };
  static constexpr Value defaultValue {{'0', '0', '0', '0', '0', '0'}};
};
SETTING_V1_WRAPPER_TYPE(ClickDurationV1, "clickDuration", 4, ClickDurationV2::Value, 0, ClickDurationV2::defaultValue);
SETTING_V1_WRAPPER_TYPE(ForceClickV1, "forceClick", 0, ForceClickV2::Value, 0, ForceClickV2::defaultValue);
SETTING_V1_WRAPPER_TYPE(MouseBaudV1, "mouseBaud", 0, MouseBaudV2::Value, 0, MouseBaudV2::defaultValue);
SETTING_V1_WRAPPER_TYPE(HostidV1, "hostid", 0, HostidV2::Value, 2, HostidV2::defaultValue);
static_assert(sizeof (ClickDurationV2::Value) == 8);
static_assert(sizeof (ForceClickV2::Value) == 4);
static_assert(sizeof (MouseBaudV2::Value) == 4);
static_assert(sizeof (HostidV2::Value) == 6);
static_assert(sizeof (ClickDurationV1) == 16);
static_assert(sizeof (ForceClickV1) == 8);
static_assert(sizeof (MouseBaudV1) == 8);
static_assert(sizeof (HostidV1) == 12);
struct Settings {
  ClickDurationV2::Value clickDuration {ClickDurationV2::defaultValue};
  ForceClickV2::Value forceClick {ForceClickV2::defaultValue};
  MouseBaudV2::Value mouseBaud {MouseBaudV2::defaultValue};
  HostidV2::Value hostid {HostidV2::defaultValue};

  inline bool operator==(const Settings &other) const {
    return this->clickDuration == other.clickDuration
      && this->forceClick == other.forceClick
      && this->mouseBaud == other.mouseBaud
      && this->hostid == other.hostid;
  }
  inline bool operator!=(const Settings& other) const {
    return !(*this == other);
  }

  uint32_t mouseBaudReal() {
    switch (mouseBaud) {
      case MouseBaud::_::S1200:
        return 1200;
      case MouseBaud::_::S2400:
        return 2400;
      case MouseBaud::_::S4800:
        return 4800;
      case MouseBaud::_::S9600:
        return 9600;
    }
    return 0;
  }

  static void begin();
  void readAll();
  template <typename SettingV1> bool readV1(SettingV1& setting);
  template <typename Setting, typename Value> bool read(Value& value);
  template <typename Setting, typename Value> void write(const Value& value);
};

extern Settings settings;

template <typename SettingV1>
bool Settings::readV1(SettingV1& setting) {
  MutexGuard m{&settingsMutex};
  SettingV1 result{};
  if (usb3sun_fs_read(SettingV1::path, reinterpret_cast<char *>(&result), sizeof result)) {
    if (result.version == 1) {
      Sprintf("settings: read %s (v1): ok\n", SettingV1::path);
      setting = result;
      return true;
    } else {
      Sprintf("settings: read %s (v1): wrong version\n", SettingV1::path);
    }
  } else {
    Sprintf("settings: read %s (v1): not found\n", SettingV1::path);
  }
  return false;
}

template <typename Setting, typename Value>
bool Settings::read(Value& value) {
  MutexGuard m{&settingsMutex};
  Value result{};
  if (usb3sun_fs_read(Setting::path, reinterpret_cast<char *>(&result), sizeof result)) {
    Sprintf("settings: read %s: ok\n", Setting::path);
    value = result;
    return true;
  } else {
    Sprintf("settings: read %s: not found\n", Setting::path);
  }
  return false;
}

template <typename Setting, typename Value>
void Settings::write(const Value& value) {
  MutexGuard m{&settingsMutex};
  if (usb3sun_fs_write(Setting::path, reinterpret_cast<const char *>(&value), sizeof value)) {
    Sprintf("settings: write %s: ok\n", Setting::path);
  } else {
    Sprintf("settings: write %s: failed\n", Setting::path);
  }
}

#endif
