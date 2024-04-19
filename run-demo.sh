#!/bin/sh
set -eu

PLATFORMIO_BUILD_FLAGS="-DSUNK_ENABLE -DSUNM_ENABLE" pio run -e native
.pio/build/native/program demo
