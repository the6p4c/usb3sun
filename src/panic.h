#ifndef USB3SUN_PANIC_H
#define USB3SUN_PANIC_H

#include "config.h"

#include "hal.h"
#include "pinout.h"

template<typename... Args>
void panic2(const char *fmt, Args... args) {
    Sprint("panic: ");
    Sprintf(fmt, args...);
    Sprintln();
    usb3sun_panic(fmt, args...);
}

#endif
