<p align="right"><strong>English</strong> · <a href="README.zh_CN.md">简体中文</a></p>

# Documentation

Sunflower Buddy is an executable Sunflower Buddy V1.4 hardware reference: stable
board APIs, a focused demo, hardware constraints, and repeatable validation in
one repository. It is not production firmware.

## Capability contract

| Capability | Implementation | Public entry point | Important boundary |
| --- | --- | --- | --- |
| Shared I2C | One ESP-IDF I2C0 bus at 400 kHz | `bsp_i2c_*` | Never create a second I2C0 owner. |
| Audio | ES8311 full-duplex PCM | `bsp_audio_*` | Reads/writes block; close codec before PMU sleep. |
| Buttons | Four active-low GPIO keys | `bsp_button_*` | Callbacks must not block. |
| Wi-Fi provisioning | BLE-only BLUFI plus persistent 2.4 GHz station configuration | `demo_blufi_*` | No embedded credentials or cloud protocol; no post-success BLE shutdown timer; reset work runs outside callbacks. |
| LED | One WS2812-compatible RGB pixel | `bsp_led_*` | `bsp_led_flash_white()` blocks for its duration. |
| Motor | MX6115 PWM plus GPIO14 ADC overcurrent monitor | `bsp_motor_*` | Duty is 0–1023; threshold requires device calibration. |
| Power | AXP2101 status, events, charging, sleep and shutdown | `bsp_power_*` | Sleep/shutdown change hardware state and are not demo actions. |
| RTC | BM8563 time, alarm IRQ and wake detection | `bsp_rtc_*` | Initialize after the shared I2C bus. |
| Wake word | GX8002 power, IRQ, version and upgrade | `bsp_wakeup_*` | Preserve rail voltages, order and 10 ms delay. |
| LTE transport | UART initialization and byte I/O | `bsp_modem_uart_*` | UART0 routing may affect console output. No PPP/cloud stack is included. |
| Deep sleep | EXT1 wake mask and coordinated PMU entry | `bsp_sleep_*` | The default mask contains four keys, PMU, GX8002 and RTC. |

Hardware constants are authoritative only in
[`bsp_pins.h`](../components/bsp/include/bsp_pins.h). Unconfirmed peripherals
are outside the capability contract.

## Document map

- [Development](development/README.md): AI workflow, environment, coding, build,
  test, flash, and result reporting.
- [BLUFI provisioning](development/engineering/blufi-provisioning.md): lifecycle,
  security boundary, mobile flow, logs, troubleshooting, and device acceptance.
- [Contribution](contribution/README.md): commit/PR and bilingual documentation
  rules.
- [Hardware design](hardware-design/README.md): specifications, hardware guide,
  constraints, troubleshooting, and device acceptance.
- [BSP API overview](../components/bsp/README.md): initialization order and
  module-level API reference.
- [Changelog](CHANGELOG.md): user-visible baseline changes.

## Repository layout

```text
components/bsp/include/  stable APIs and the authoritative pin map
components/bsp/src/      board resource ownership and hardware implementations
main/                    one runnable record/playback and BLUFI reference demo
main/data/gx_bl/         preserved GX8002 boot and wake-word firmware
docs/                    bilingual engineering and hardware documentation
tests/                   repository-contract host tests
tools/                   local static and isolated firmware validation
```
