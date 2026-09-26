# Pixel Clock ESP

ESP32-C6 firmware for a pixel clock. Boot mounts an asset partition and
presents a test bitmap on the LED matrix. The platform runtime is a single
control task, a bounded event/command mailbox, and a timer scheduler. Wi-Fi,
BLE, weather, and OTA are not implemented yet.

Target: `esp32c6`. Language: C++. Build: ESP-IDF with `MINIMAL_BUILD`.

## Layout

- `main/` — composition root (`app_main`)
- `components/platform/` — component interface, mailbox, runtime, scheduler, logging
- `components/graphics/` — framebuffer, element tree, and bitmap rendering
- `components/display/` — LED matrix output; loads the test bitmap and presents it
- `components/<name>/test/native_host/` — GoogleTest cases owned by that component
- `components/<name>/test/embedded/` — Unity cases and mocks owned by that component
- `test/native_host/` — shared GoogleTest/CTest runner (no ESP-IDF; no `test_*.cpp`)
- `test/embedded/` — shared Unity runner app (cross-compiled for ESP32; not firmware)

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

On boot, `app_main` mounts the assets partition, registers `DisplayRuntime`,
and arms a coalesced `RefreshDisplay` timer at 15 Hz before `runtime.start()`.
`esp_timer` fires the first callback one period later (about 66.7 ms), after
`DisplayRuntime::start` has loaded `/assets/test.bmp`, built one root bitmap
element at the origin, and initialized the LED matrix. Each event clears the
32×8 framebuffer, renders that element, and presents the frame. The event
`coalesce_key` replaces a queued refresh when a frame is still waiting, so the
mailbox keeps a single pending frame. The control task runs every step.

## Tests

Tests are split into native-host and embedded suites. Use native-host GoogleTest
for code that does not depend on ESP-IDF, FreeRTOS, peripherals, or a specific
chip. Use the embedded Unity app for behavior that needs the ESP environment,
QEMU, or hardware. Cases live next to the component
(`components/<name>/test/native_host/` or `.../embedded/`); `test/native_host/`
and `test/embedded/` are shared runners only. The firmware `project()` does not
compile `components/*/test/`; the Unity runner pulls those cases in via
`TEST_COMPONENTS`.

### All tests

After installing the QEMU RISC-V binary (see below), run both the native-host
and embedded QEMU suites from the repository root:

```bash
./test/run_all.sh
```

The script uses the active ESP-IDF environment, or runs the embedded suite
through EIM when ESP-IDF is not already active.

### Native host (GoogleTest)

From the repository root (no ESP-IDF required):

```bash
cmake -S test/native_host -B test/native_host/build
cmake --build test/native_host/build
ctest --test-dir test/native_host/build --output-on-failure
```

In VS Code, run the **Test (native host)** task.

### Embedded (QEMU)

QEMU does not emulate ESP32-C6, so the embedded suite builds for ESP32-C3
(closest RISC-V target) into `test/embedded/build_esp32c3_qemu/`. That does
not replace the C6 on-device build under `test/embedded/build/`. Install the
QEMU RISC-V binary once, then from `test/embedded/`:

```bash
eim run 'python $IDF_PATH/tools/idf_tools.py install qemu-riscv32'
eim run "idf.py -B build_esp32c3_qemu -D SDKCONFIG=build_esp32c3_qemu/sdkconfig -D IDF_TARGET=esp32c3 build"
eim run "pytest pytest_embedded_tests.py"
```

In VS Code, run the **Test (embedded QEMU)** task.

### Embedded (on device, ESP32-C6)

From `test/embedded/`:

```bash
eim run "idf.py set-target esp32c6"
eim run "idf.py -p /dev/cu.usbmodem1101 flash monitor"
```

In VS Code, run the **Test (embedded on device)** task.

`set-target` is only needed on a fresh tree (or after a QEMU/C3 configure in the
default build dir). After boot, press Enter for the Unity menu. `*` runs every
case, `[mailbox]` / `[scheduler]` a tag, or a number for one case. Quit with
`Ctrl+]`.
