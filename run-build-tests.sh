#!/bin/sh
set -eu

build() {
    pio run -t clean
    pio run "$@"
}

# build with no flags.
export PLATFORMIO_BUILD_FLAGS= # no flags
build "$@"

# build with feature flags.
export PLATFORMIO_BUILD_FLAGS='
    -DSUNK_ENABLE
    -DSUNM_ENABLE
    -DWIPE_SETTINGS
    -DDEBUG_TIMINGS
    -DUHID_LED_ENABLE
    -DUHID_LED_TEST
    -DWAIT_PIN
    -DWAIT_SERIAL
'; build "$@"

# build with feature flags and verbose flags.
export PLATFORMIO_BUILD_FLAGS='
    -DSUNK_ENABLE
    -DSUNM_ENABLE
    -DWIPE_SETTINGS
    -DDEBUG_TIMINGS
    -DUHID_LED_ENABLE
    -DUHID_LED_TEST
    -DWAIT_PIN
    -DWAIT_SERIAL
    -DBUZZER_VERBOSE
    -DSUNK_VERBOSE
    -DSUNM_VERBOSE
    -DUHID_VERBOSE
'; build "$@"
