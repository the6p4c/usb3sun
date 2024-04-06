#ifndef USB3SUN_MUTEX_H
#define USB3SUN_MUTEX_H

#include "hal.h"

struct MutexGuard {
    MutexGuard(usb3sun_mutex *mutex) : mutex(mutex) {
        // TODO port deadlock detection from CoreMutex?
        usb3sun_mutex_lock(mutex);
    }

    ~MutexGuard() {
        usb3sun_mutex_unlock(mutex);
    }

    usb3sun_mutex *mutex;
};

#endif
