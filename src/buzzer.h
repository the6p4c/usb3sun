#ifndef USB3SUN_BUZZER_H
#define USB3SUN_BUZZER_H

#include <atomic>
#include <optional>

#include "hal.h"

struct Buzzer {
  inline static const unsigned long plugDuration = 125'000u;
  inline static const unsigned long unplugDuration = 125'000u;
  inline static const unsigned int bellPitch = 1'000'000u / 480u; // 480 us period
  inline static const unsigned int plugPitch = 261.6255653005986346778499935233; // C4
  inline static const unsigned int plugPitch2 = 391.99543598174929408569953045983; // G4

  typedef enum class State : int {
    NONE,
    BELL,
    CLICK,
    PLUG,
    PLUG2,
    UNPLUG,
    UNPLUG2,
  } _;

  State current;
  unsigned long since;
  std::optional<unsigned long> temporaryClickDuration{};

  void update();
  void click(std::optional<unsigned long> temporaryDuration = {});
  void plug();
  void unplug();

private:
  bool isExpired(unsigned long t, unsigned long duration);
  void setCurrent(unsigned long t, State value);
  void update0();
  void pwmTone(unsigned int pitch, std::optional<unsigned long> duration = {});
  unsigned long clickDuration() const;
};

extern Buzzer buzzer;
extern usb3sun_mutex buzzerMutex;

#endif
