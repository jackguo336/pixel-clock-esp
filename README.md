# Pixel Clock ESP

ESP32-C6 firmware for a pixel clock. This tree currently contains only the
platform runtime kernel: a single control task, a bounded event/command
mailbox, and a timer scheduler. Domain services (Wi-Fi, BLE, weather, display,
OTA) are not implemented yet.

Target: `esp32c6`. Language: C++. Build: ESP-IDF with `MINIMAL_BUILD`.

## Layout

- `main/` — composition root (`app_main`) and temporary TickSource/TickSink demo
- `components/platform/` — component interface, mailbox, runtime, scheduler, logging
- `test/` — Unity test app (not compiled into firmware); cases live in `components/platform/test/`

## Build

ESP-IDF v6 via EIM (selected version `v6.0.2`):

```bash
eim run "idf.py set-target esp32c6"
eim run "idf.py build"
eim run "idf.py -p /dev/cu.usbmodem1101 flash monitor"
```

`set-target` is only needed on a fresh tree (or after changing chips). Quit the
serial monitor with `Ctrl+]`.

## Runtime demo

On boot, `TickSource` posts `Tick` events on a periodic timer. `TickSink` logs
each tick and, after a few counts, unicasts `PauseTicks`. `TickSource` cancels
the timer and posts `TicksPaused`. Everything runs on the control task; the
stubs will be removed when real services land.

## Unit tests

The firmware `project()` does not compile `components/*/test/`. A sibling app
under `test/` pulls those in via `TEST_COMPONENTS` and runs Unity.

### Local (QEMU)

QEMU does not emulate ESP32-C6, so the host suite builds for ESP32-C3 (closest
RISC-V target) into `test/build_esp32c3_qemu/`. That does not replace the C6
on-device build under `test/build/`. Install the QEMU RISC-V binary once, then
from `test/`:

```bash
eim run 'python $IDF_PATH/tools/idf_tools.py install qemu-riscv32'
eim run "idf.py -B build_esp32c3_qemu -D SDKCONFIG=build_esp32c3_qemu/sdkconfig -D IDF_TARGET=esp32c3 build"
eim run "pytest pytest_unit_tests.py"
```

### On device (ESP32-C6)

From `test/`:

```bash
eim run "idf.py set-target esp32c6"
eim run "idf.py -p /dev/cu.usbmodem1101 flash monitor"
```

`set-target` is only needed on a fresh tree (or after a QEMU/C3 configure in the
default build dir). After boot, press Enter for the Unity menu. `*` runs every
case, `[mailbox]` / `[scheduler]` a tag, or a number for one case. Quit with
`Ctrl+]`.
