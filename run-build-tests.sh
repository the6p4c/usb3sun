#!/bin/sh
set -eu

build_with() {
    pio run -t clean
    PLATFORMIO_BUILD_FLAGS="$*" pio run
}

# build with no flags.
build_with # no flags

# build with feature flags.
build_with \
    -DSUNK_ENABLE \
    -DSUNM_ENABLE \
    -DWIPE_SETTINGS \
    -DDEBUG_TIMINGS \
    -DUHID_LED_ENABLE \
    -DUHID_LED_TEST \
    -DWAIT_PIN \
    -DWAIT_SERIAL \
;

# build with feature flags and verbose flags.
build_with \
    -DSUNK_ENABLE \
    -DSUNM_ENABLE \
    -DWIPE_SETTINGS \
    -DDEBUG_TIMINGS \
    -DUHID_LED_ENABLE \
    -DUHID_LED_TEST \
    -DWAIT_PIN \
    -DWAIT_SERIAL \
    -DBUZZER_VERBOSE \
    -DSUNK_VERBOSE \
    -DSUNM_VERBOSE \
    -DUHID_VERBOSE \
;
