<p align="right"><strong>English</strong> · <a href="build-and-test.zh_CN.md">简体中文</a></p>

# Build and Test

Use ESP-IDF 5.5.3 and target ESP32-S3.

## Incremental build

```bash
source /path/to/esp-idf-v5.5.3/export.sh
idf.py build
```

Run `idf.py set-target esp32s3` once for a new checkout or when deliberately
changing generated target state. The top-level CMake file also forces ESP32-S3;
repeating `set-target` needlessly regenerates `sdkconfig`.

Expected outputs include:

- `build/sunflower-buddy.bin`: application image for offset `0x10000`;
- `build/storage.bin`: GX8002 firmware LittleFS image for offset `0x420000`;
- bootloader and partition-table images used by `idf.py flash`.

Do not flash the application image at offset `0x0`.

## Validation

Validation entry points:

```bash
./tools/validate.sh --static
./tools/validate.sh --firmware
./tools/validate.sh
```

Static validation checks the single-board BSP/demo contract and host tests.
Firmware validation uses an isolated
temporary build directory and leaves the developer's root `sdkconfig` and
`build/` untouched. Its temporary firmware output is deleted after validation;
run `idf.py build` when persistent flash artifacts are needed.
The first firmware validation may download locked managed components and
therefore needs registry/Git access.

`dependencies.lock` is committed. After changing an `idf_component.yml`, build
with ESP-IDF 5.5.3, inspect dependency changes, and commit the manifest and lock
file together. Do not edit `managed_components/`.

## Flash and monitor

```bash
idf.py -p <port> flash monitor
```

The equivalent offsets for the current partition layout are bootloader `0x0`,
partition table `0x8000`, application `0x10000`, and LittleFS storage `0x420000`.
Prefer `idf.py flash` so generated flash arguments remain authoritative.

## Reading build and boot output

- Find the first `FAILED`, `fatal error`, or CMake diagnostic. The final
  `ninja: build stopped` line only reports that an earlier command failed.
- A bootloader that builds successfully does not prove the application linked;
  confirm `sunflower-buddy.bin` and the final partition-size check.
- The boot log should identify project `sunflower-buddy`, the intended
  Git-derived app version, ESP-IDF 5.5.3, 16 MB Flash, and 8 MB octal PSRAM.
- Deprecated `esptool` option warnings emitted by an upstream build command are
  not failures when image generation and size checks complete successfully.
- `Incorrect size of core dump image` at boot commonly means the coredump
  partition contains stale/non-coredump bytes. It does not describe the current
  boot as a crash; erase that dedicated partition only when its old contents are
  no longer needed.

## Result reporting

Report these independently:

1. **Build:** ESP-IDF version, command, target, and result.
2. **Host tests:** command and test count/result.
3. **Device tests:** board revision, firmware commit, observed behavior, and logs.
4. **Unverified:** every relevant physical behavior that was not tested.

For Wi-Fi/BLUFI changes, include the mobile-app version and the cleared/saved
station configuration, saved-boot, disconnect/retry, long-press reset, and
repeated-session results from the [BLUFI guide](blufi-provisioning.md).

For device testing use a dedicated development board, then validate the relevant
hardware items in the [Sunflower Buddy guide](../../hardware-design/SUNFLOWER_BUDDY_V14_HARDWARE_GUIDE.md).
