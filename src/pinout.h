#ifndef USB3SUN_PINOUT_H
#define USB3SUN_PINOUT_H

#include "config.h"

#include "hal.h"

struct Pinout {
  Pinout();
  void begin();
  void beginSun();
  void restartSunm();
  bool debugWrite(const char *data, size_t len);
  bool debugPrint(const char *text);
  bool debugPrintln();
  bool debugPrintln(const char *text);
  bool debugPrintf(const char *format, ...) __attribute__ ((format (printf, 2, 3)));

private:
  void v1();
  void v2();
  void allowDebugOverCdc();
  void allowDebugOverUart();
};

extern Pinout pinout;

#endif
