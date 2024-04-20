#!/bin/sh
set -eu

describe() {
    git describe --match='*.*' --dirty=+
}
make_version() {
    sed -E 's/([^-]+(-[^-]+)?)(-[^-+]+)?([+]?)/\1\4/;s/-/+/;s/.*/-DUSB3SUN_VERSION=\\"&\\"/'
}
die() {
    >&2 echo 'fatal: tests failed in get-version.sh!'
    exit 1
}

# to debug failures: sh -x ./get-version.sh
[ "$(echo '1.5' | make_version)" = '-DUSB3SUN_VERSION=\"1.5\"' ] || die
[ "$(echo '1.5+' | make_version)" = '-DUSB3SUN_VERSION=\"1.5+\"' ] || die
[ "$(echo '1.5-137-gd41d98a' | make_version)" = '-DUSB3SUN_VERSION=\"1.5+137\"' ] || die
[ "$(echo '1.5-137-gd41d98a+' | make_version)" = '-DUSB3SUN_VERSION=\"1.5+137+\"' ] || die

if describe > /dev/null 2>&1; then
    describe | make_version
else
    echo '-DUSB3SUN_VERSION=\"?\"'
fi
