#!/bin/sh
set -eu

for ke in -DSUNK_ENABLE ''; do
for me in -DSUNM_ENABLE ''; do
    PLATFORMIO_BUILD_FLAGS="$ke $me" pio run -e native
    .pio/build/native/program ${1-all}
done
done
