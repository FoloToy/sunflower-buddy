<p align="right"><strong>English</strong> · <a href="specifications.zh_CN.md">简体中文</a></p>

# Specifications

This page lists capabilities confirmed by the current board adaptation. It does
not claim every feature the ESP32-S3 or individual peripheral chips could
theoretically provide.

| Area | Sunflower Buddy V1.4 baseline |
| --- | --- |
| Software baseline | ESP-IDF 5.5.3, target fixed to ESP32-S3 |
| MCU and memory | ESP32-S3, 16 MB Flash, 8 MB octal PSRAM |
| Radio reference | 2.4 GHz Wi-Fi station plus BLE 4.2 BLUFI; saved-config auto-connect and 1–30 second reconnect backoff |
| Audio | ES8311 codec, microphone input, speaker output, 24 kHz mono demo |
| Controls | Four active-low keys: Wi-Fi/mode, voice, volume up, volume down |
| Feedback | One addressable RGB LED and MX6115-driven motor |
| Power | AXP2101 PMIC with battery status, charging events/control, sleep and shutdown |
| Time | BM8563 RTC with alarm interrupt and deep-sleep wake support |
| Wake word | GX8002 with dedicated rails, interrupt, version query, enable/disable and firmware upgrade |
| Cellular link | LTE module UART; 115200 known startup baud and 921600 target baud |
| Shared control bus | I2C0 at 400 kHz for AXP2101, BM8563, GX8002 and ES8311 |
| Storage layout | 24 KB NVS, 4 KB PHY data, 4 MB factory app, 64 KB coredump, 512 KB LittleFS; remaining Flash is unpartitioned |
| Demo | Hold-to-record/release-to-play, BLUFI provisioning/reset, 10% volume steps, LED/motor feedback and status logging |

The repository intentionally excludes product/cloud state machines, OTA,
embedded credentials, and unconfirmed peripherals. BLUFI example encryption is
not a claim of production identity, BLE SMP, or encrypted NVS support.
