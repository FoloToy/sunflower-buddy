<p align="right"><strong>English</strong> · <a href="README.zh_CN.md">简体中文</a></p>

# Sunflower Buddy

An ESP-IDF hardware-reference project for the FoloToy Sunflower Buddy V1.4 board. It is
intentionally small: stable board APIs, basic implementations, and runnable
examples that AI coding assistants can use as a development baseline.

This is not the production FoloToy firmware. MQTT, OTA, product state machines,
embedded credentials, media assets, and cloud protocols are intentionally excluded.

## Structure

```text
components/bsp/include/  Stable hardware APIs and the authoritative pin map
components/bsp/src/      Complete Sunflower Buddy V1.4 board-level hardware adaptation
main/                    Button/audio demo and BLUFI provisioning behavior
docs/                    Hardware facts and development guidance
tests/                   Repository-contract host tests
tools/                   Local and CI validation
```

Reusable hardware code stays in `components/bsp`; application behavior stays
in `main`.

## Hardware baseline

ESP32-S3 with 16 MB Flash and 8 MB octal PSRAM drives ES8311 audio, four keys,
one RGB LED, an MX6115 motor, AXP2101 power management, BM8563 RTC, GX8002 wake
word processing, and an LTE UART transport. See the
[capability contract](docs/README.md) and [BSP API overview](components/bsp/README.md)
before extending the demo.

## Demo controls

- Hold the voice button to record; release it to play the recording back while
  the motor runs continuously.
- GX8002 wake-word event: run the motor at full speed for 500 ms.
- Wi-Fi button click: print board/Wi-Fi status and run a white LED/motor self-test.
- Wi-Fi button hold (1 second): clear saved Wi-Fi credentials and restart BLUFI
  provisioning; an existing BLUFI client is disconnected and must reconnect.
- Volume buttons: adjust playback volume by 10%, limited to 0%–100%.

The single demo uses the microphone, speaker, RGB LED, motor, buttons, PMU,
GX8002, Wi-Fi, and BLE. A recording can last up to 15 seconds and is buffered in
PSRAM. Results are printed to the serial log.

## BLUFI provisioning

On boot, the demo automatically connects to 2.4 GHz Wi-Fi with the station
configuration saved by the ESP-IDF driver. Disconnects are retried indefinitely
with a 1–30 second exponential backoff. If no configuration exists, BLUFI
advertises as `SunflowerBuddy-XXXX`, where `XXXX` is derived from the Bluetooth
MAC address. Use an Espressif-compatible BLUFI app to scan access points and
send credentials.

The BLUFI transport uses the ESP-IDF example's DH/AES negotiation and caps the
GATT MTU for one-byte BLUFI frame lengths; password contents are never logged.
After Wi-Fi succeeds, advertising does not reopen after the client disconnects,
but an already-connected GATT client is retained and there is currently no BLE
shutdown timer. See the [BLUFI provisioning guide](docs/development/engineering/blufi-provisioning.md)
for the lifecycle, security boundary, logs, reset workflow, and acceptance
tests. This feature provisions local Wi-Fi only and adds no cloud protocol.

The BSP also retains hardware APIs not exercised destructively by the demo:
AXP2101 voltages, charge state, power sources, IRQs, charge control, sleep and
shutdown; BM8563 time and alarms; all four button actions;
MX6115 mode, speed and overcurrent protection; ES8311 lifecycle; LTE UART; and
deep-sleep wake sources.

The Sunflower 2 GX8002 wake-word firmware is packaged in the `storage` LittleFS
partition and mounted at `/littlefs/gx_bl`. It is preserved for explicit
recovery or upgrade through `bsp_wakeup_upgrade()`; the demo does not rewrite
the GX8002 automatically on every boot.

## Build

```bash
source /path/to/esp-idf-v5.5.3/export.sh
# Run once for a new checkout or after changing the target.
idf.py set-target esp32s3
idf.py build
```

Run the repository gates with:

```bash
./tools/validate.sh --static
./tools/validate.sh --firmware
```

Flash and monitor a connected development unit with:

```bash
idf.py -p <port> flash monitor
```

Start at the [documentation index](docs/README.md). Environment setup, coding
rules, hardware constraints, troubleshooting, and device acceptance are linked
there. Contributors and coding agents must also follow [AGENTS.md](AGENTS.md).

Physical-device behavior still requires verification on Sunflower Buddy V1.4 hardware.
