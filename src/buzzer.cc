#include "config.h"
#include "buzzer.h"

#include "hal.h"
#include "mutex.h"
#include "settings.h"
#include "state.h"

void Buzzer::update0() {
  const auto t = usb3sun_micros();

  // starting tone is not entirely idempotent, so avoid restarting it.
  // spamming it every 10 ms will just pop and then silence in practice.
  switch (current) {
    case _::NONE:
      break;
    case _::BELL:
      if (state.bell) {
        return;
      }
      break;
    case _::CLICK:
      if (!isExpired(t, clickDuration() * 1'000uL)) {
        return;
      }
      temporaryClickDuration = {};
      break;
    case _::PLUG:
      if (!isExpired(t, plugDuration)) {
        return;
      } else {
        setCurrent(t, Buzzer::_::PLUG2);
        usb3sun_buzzer_start(plugPitch2);
        return;
      }
      break;
    case _::PLUG2:
      if (!isExpired(t, plugDuration)) {
        return;
      }
      break;
    case _::UNPLUG:
      if (!isExpired(t, plugDuration)) {
        return;
      } else {
        setCurrent(t, Buzzer::_::UNPLUG2);
        usb3sun_buzzer_start(plugPitch);
        return;
      }
      break;
    case _::UNPLUG2:
      if (!isExpired(t, plugDuration)) {
        return;
      }
      break;
  }
  if (state.bell) {
    setCurrent(t, Buzzer::_::BELL);
    usb3sun_buzzer_start(bellPitch);
  } else if (current != Buzzer::_::NONE) {
    setCurrent(t, Buzzer::_::NONE);
    usb3sun_gpio_write(BUZZER_PIN, false);
  }
}

bool Buzzer::isExpired(unsigned long t, unsigned long duration) {
  return t - since >= duration || since < duration;
}

void Buzzer::setCurrent(unsigned long t, Buzzer::State value) {
#ifdef BUZZER_VERBOSE
  Sprintf("buzzer: setCurrent %d\n", static_cast<int>(value));
#endif
  current = value;
  since = t;
}

unsigned long Buzzer::clickDuration() const {
  return temporaryClickDuration.value_or(settings.clickDuration);
}

void Buzzer::update() {
  MutexGuard m{&buzzerMutex};
  update0();
}

void Buzzer::click(std::optional<unsigned long> temporaryDuration) {
  MutexGuard m{&buzzerMutex};
  if (!temporaryDuration.has_value()) {
    switch (settings.forceClick) {
      default:
      case ForceClick::_::NO:
        if (!state.clickEnabled) return;
        break;
      case ForceClick::_::OFF:
        return;
      case ForceClick::_::ON:
        break;
    }
  }

  if (current <= Buzzer::_::CLICK) {
    this->temporaryClickDuration = temporaryDuration;
    // violation of sparc keyboard spec :) but distinguishable from bell!
    setCurrent(usb3sun_micros(), Buzzer::_::CLICK);
    usb3sun_buzzer_start(1'000);
  }
}

void Buzzer::plug() {
  MutexGuard m{&buzzerMutex};
  if (current <= Buzzer::_::PLUG2) {
    setCurrent(usb3sun_micros(), Buzzer::_::PLUG);
    usb3sun_buzzer_start(plugPitch);
  }
}

void Buzzer::unplug() {
  MutexGuard m{&buzzerMutex};
  if (current <= Buzzer::_::UNPLUG2) {
    setCurrent(usb3sun_micros(), Buzzer::_::UNPLUG);
    usb3sun_buzzer_start(plugPitch2);
  }
}
