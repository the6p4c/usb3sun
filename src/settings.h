#ifndef USB3SUN_SETTINGS_H
#define USB3SUN_SETTINGS_H

#include <algorithm>
#include <cstdint>
#include <cstring>

#include "hal.h"
#include "mutex.h"
#include "pinout.h"

#define SETTING(_name, _version, _type, ...) \
  struct _name##Setting { \
    static const unsigned currentVersion = _version; \
    static constexpr const char *const path = "/" #_name; \
    unsigned version = currentVersion; \
    _type value = __VA_ARGS__; \
    inline bool operator==(const _name##Setting &other) const { \
      return this->value == other.value; \
    } \
  } _name##_field; \
  _type &_name() { \
    return _name##_field.value; \
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
struct Settings {
  SETTING(clickDuration, 1, unsigned long, 5uL); // [0,100]
  SETTING(forceClick, 1, ForceClick, {ForceClick::_::NO});
  SETTING(mouseBaud, 1, MouseBaud, {MouseBaud::_::S9600});
  SETTING(hostid, 1, Hostid, {'0', '0', '0', '0', '0', '0'});

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
  template <typename T> void read(T& setting);
  template <typename T> void write(const T& setting);
};

extern Settings settings;

template <typename T>
void Settings::read(T& setting) {
  MutexGuard m{&settingsMutex};
  T result{};
  if (usb3sun_fs_read(T::path, reinterpret_cast<char *>(&result), sizeof result)) {
    if (result.version == T::currentVersion) {
      Sprintf("settings: read %s: version %u\n", T::path, T::currentVersion);
      setting = result;
      return;
    } else {
      Sprintf("settings: read %s: wrong version\n", T::path);
    }
  } else {
    Sprintf("settings: read %s: file not found\n", T::path);
  }
  setting = T{};
}

template <typename T>
void Settings::write(const T& setting) {
  MutexGuard m{&settingsMutex};
  if (usb3sun_fs_write(T::path, reinterpret_cast<const char *>(&setting), sizeof setting)) {
    Sprintf("settings: write %s: version %u\n", T::path, T::currentVersion);
  } else {
    Sprintf("settings: write %s: failed\n", T::path);
  }
}

#endif
