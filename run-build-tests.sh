#!/bin/sh
set -eu

build() {
    pio run -t clean
    pio run "$@"
}

# build with all flags off.
export PLATFORMIO_BUILD_FLAGS='
    -USUNK_ENABLE
    -USUNM_ENABLE
    -UWIPE_SETTINGS
    -UDEBUG_TIMINGS
    -UUHID_LED_ENABLE
    -UUHID_LED_TEST
    -UWAIT_PIN
    -UWAIT_SERIAL
    -UBUZZER_VERBOSE
    -USUNK_VERBOSE
    -USUNM_VERBOSE
    -UUHID_VERBOSE
';
build "$@"

# build with feature flags on, but verbose flags off.
export PLATFORMIO_BUILD_FLAGS='
    -DSUNK_ENABLE
    -DSUNM_ENABLE
    -DWIPE_SETTINGS
    -DDEBUG_TIMINGS
    -DUHID_LED_ENABLE
    -DUHID_LED_TEST
    -DWAIT_PIN
    -DWAIT_SERIAL
    -UBUZZER_VERBOSE
    -USUNK_VERBOSE
    -USUNM_VERBOSE
    -UUHID_VERBOSE
'; build "$@"

# build with all flags on.
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
