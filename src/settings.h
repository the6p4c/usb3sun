#ifndef USB3SUN_SETTINGS_H
#define USB3SUN_SETTINGS_H

#include <algorithm>
#include <cstdint>
#include <cstring>

#include "hal.h"
#include "mutex.h"
#include "pinout.h"

#define SETTING_V1_WRAPPER_TYPE(_wrapper_name, _file_name, _payload_type, ...) \
  struct _wrapper_name { \
    static const unsigned currentVersion = 1; \
    static constexpr const char *const path = "/" _file_name; \
    unsigned version = currentVersion; \
    _payload_type value = __VA_ARGS__; \
    inline bool operator==(const _wrapper_name &other) const { \
      return this->value == other.value; \
    } \
  };

#define SETTING_V1_WRAPPER_FIELD(_wrapper_name, _field_name, _payload_type) \
  _wrapper_name _field_name##_field; \
  _payload_type &_field_name() { \
    return _field_name##_field.value; \
  }

#define SETTING_ENUM(_name, ...) \
struct _name { \
  typedef enum class State: int { __VA_ARGS__, VALUE_COUNT } _; \
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
struct Hostid {
  unsigned char value[6];
  inline bool operator==(const Hostid &other) const {
    return !memcmp(value, other.value, sizeof value);
  }
  inline bool operator!=(const Hostid &other) const {
    return !(*this == other);
  }
};
SETTING_V1_WRAPPER_TYPE(ClickDurationV1, "clickDuration", unsigned long, 5uL); // [0,100]
SETTING_V1_WRAPPER_TYPE(ForceClickV1, "forceClick", ForceClick, {ForceClick::_::NO});
SETTING_V1_WRAPPER_TYPE(MouseBaudV1, "mouseBaud", MouseBaud, {MouseBaud::_::S9600});
SETTING_V1_WRAPPER_TYPE(HostidV1, "hostid", Hostid, {'0', '0', '0', '0', '0', '0'});
struct Settings {
  SETTING_V1_WRAPPER_FIELD(ClickDurationV1, clickDuration, unsigned long);
  SETTING_V1_WRAPPER_FIELD(ForceClickV1, forceClick, ForceClick);
  SETTING_V1_WRAPPER_FIELD(MouseBaudV1, mouseBaud, MouseBaud);
  SETTING_V1_WRAPPER_FIELD(HostidV1, hostid, Hostid);

  inline bool operator==(const Settings &other) const {
    return this->clickDuration_field == other.clickDuration_field
      && this->forceClick_field == other.forceClick_field
      && this->mouseBaud_field == other.mouseBaud_field
      && this->hostid_field == other.hostid_field;
  }
  inline bool operator!=(const Settings& other) const {
    return !(*this == other);
  }

  uint32_t mouseBaudReal() {
    switch (mouseBaud()) {
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

  const unsigned char *hostidRef() const {
    return hostid_field.value.value;
  }
  unsigned char *hostidMut() {
    return hostid_field.value.value;
  }

  static void begin();
  void readAll();
  template <typename SettingV1> void readV1(SettingV1& setting);
  template <typename Setting> void write(const Setting& setting);
};

extern Settings settings;

template <typename SettingV1>
void Settings::readV1(SettingV1& setting) {
  MutexGuard m{&settingsMutex};
  SettingV1 result{};
  if (usb3sun_fs_read(SettingV1::path, reinterpret_cast<char *>(&result), sizeof result)) {
    if (result.version == SettingV1::currentVersion) {
      Sprintf("settings: read %s: version %u\n", SettingV1::path, SettingV1::currentVersion);
      setting = result;
      return;
    } else {
      Sprintf("settings: read %s: wrong version\n", SettingV1::path);
    }
  } else {
    Sprintf("settings: read %s: file not found\n", SettingV1::path);
  }
  setting = SettingV1{};
}

template <typename Setting>
void Settings::write(const Setting& setting) {
  MutexGuard m{&settingsMutex};
  if (usb3sun_fs_write(Setting::path, reinterpret_cast<const char *>(&setting), sizeof setting)) {
    Sprintf("settings: write %s: version %u\n", Setting::path, Setting::currentVersion);
  } else {
    Sprintf("settings: write %s: failed\n", Setting::path);
  }
}

#endif
