[README](../README.md) >

firmware
========

usb3sun uses [Nix](https://nixos.org/download/) (the package manager) to get exact versions of its dependencies.
you can build the firmware without Nix, but your mileage may vary.

there are two build environments, `pico` and `linux`.
`pico` is the real firmware that runs on a real usb3sun adapter.
`linux` runs on an ordinary linux machine, for the test suite and interactive demo.

## building the firmware for `pico`

1. `git apply picopiousb1.patch` ([picopiousb1.patch](../picopiousb1.patch), [#12](https://github.com/delan/usb3sun/issues/12))\
→ improves compatibility with Microsoft Wired Keyboard 600 (045E:0750)
2. `git apply tinyusb3.patch` ([tinyusb3.patch](../tinyusb3.patch), [#13](https://github.com/delan/usb3sun/issues/13))\
→ fixes a bug where dummy event callbacks shadow our own callbacks ([Adafruit_TinyUSB_Arduino#296](https://github.com/adafruit/Adafruit_TinyUSB_Arduino/issues/296))
3. `git apply debug1.patch` ([debug1.patch](../debug1.patch))\
→ makes TinyUSB debug logging configurable at runtime
4. `git -C ~/.platformio/packages/framework-arduinopico apply $PWD/debug2.patch` ([debug2.patch](../debug2.patch))\
→ makes arduino-pico debug logging configurable at runtime
5. `git -C ~/.platformio/packages/framework-arduinopico apply $PWD/debug3.patch` ([debug3.patch](../debug3.patch))\
→ makes TinyUSB debug logging configurable at runtime
6. (if using Nix) `nix-shell`
7. `pio run -e pico`

## building the firmware for `linux`

1. (if using Nix) `nix-shell`
2. `pio run -e linux`

## how to run the interactive demo

the interactive demo can only be built for the `linux` environment, and uses a named pipe (fifo(7)) to emulate the display:

```sh
$ ./run-demo.sh                     # run, creating a random fifo in /tmp
$ ./run-demo.sh <path/to/fifo>      # run, creating a fifo at the given path
```

to get a live view of the emulated display, open another terminal, and cat the same path you gave in a loop:

```sh
$ while :; do cat <path/to/fifo>; done
```

note that due to the way named pipes work, the demo will pause until you start reading from the fifo, and will get killed with SIGPIPE if you stop reading from the fifo.

input in the terminal where you ran the demo is (roughly and incompletely) translated to usb keyboard input, with a couple of exceptions:

- **Alt+Space** becomes **Right Ctrl+Space**, opening the menu
- **q** quits the demo

## how to run the tests

the main test suite can only be built for the `linux` environment, and automatically runs multiple times to test every combination of -DSUNK_ENABLE and -DSUNM_ENABLE:

```sh
$ ./run-tests.sh                    # run all tests
$ ./run-tests.sh all                # (same as above)
$ ./run-tests.sh <test>             # run a specific test
$ pio run -e linux -t exec          # list available tests
```

the build tests compile the firmware with a few different sets of build flags, to ensure that they all build without errors (and show you the warnings for each):

```sh
$ ./run-build-tests.sh              # both pico and linux
$ ./run-build-tests.sh -e pico      # pico only
$ ./run-build-tests.sh -e linux     # linux only
```

## general troubleshooting

`*** [.pio/build/pico/firmware.elf] ModuleNotFoundError : No module named 'SCons.Tool.FortranCommon'`
— in vscode with the platformio extension, this can happen if you start building the firmware too quickly after making changes to platformio.ini.
wait a moment, then try again.

## linux troubleshooting

if you get “patch does not apply” errors:

```sh
$ dos2unix '.pio/libdeps/pico/Adafruit TinyUSB Library/src/host/usbh.c'
```

copy these commands to do all of the above:

```sh
dos2unix '.pio/libdeps/pico/Adafruit TinyUSB Library/src/host/usbh.c'
git apply tinyusb1.patch
git apply tinyusb2.patch
git apply debug1.patch
git -C ~/.platformio/packages/framework-arduinopico apply $PWD/debug2.patch
```

## windows troubleshooting

if you encounter problems with paths being too long:

1. open an admin command prompt (Win+X, A)
2. `git config --system core.longpaths true`
3. whenever you get `[WinError 3]` or `[WinError 145]`, delete `%USERPROFILE%\.platformio\.cache\tmp`

if you get “patch does not apply” errors in step 2:

1. change the line endings from CRLF to LF in <.pio/libdeps/pico/Adafruit TinyUSB Library/src/host/usbh.c>

## how to get clangd (LSP) working

```sh
$ pio run -t compiledb
```
