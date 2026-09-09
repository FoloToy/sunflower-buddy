<p align="right"><strong>English</strong> · <a href="README.zh_CN.md">简体中文</a></p>

# Hardware Design

This directory owns firmware-visible Sunflower Buddy V1.4 hardware facts, resource
constraints, initialization order, troubleshooting, and device acceptance.

## Document map

| Document | Authority |
| --- | --- |
| [Specifications](specifications.md) | Confirmed board capabilities and software support boundary. |
| [Sunflower Buddy V1.4 hardware guide](SUNFLOWER_BUDDY_V14_HARDWARE_GUIDE.md) | Pin use, buses, sequencing, runtime constraints, troubleshooting, and acceptance. |
| [`bsp_pins.h`](../../components/bsp/include/bsp_pins.h) | Single source of firmware pin, address, clock, and board-configuration constants. |
| [BSP README](../../components/bsp/README.md) | Public API, ownership, initialization, and call-context reference. |
| [BLUFI provisioning guide](../development/engineering/blufi-provisioning.md) | Application-level Wi-Fi/BLE lifecycle, security boundary, diagnostics, and RF acceptance. |

Do not infer board wiring from generic ESP32-S3 capabilities or another FoloToy
board. A build proves software compatibility only; physical behavior requires a
Sunflower Buddy V1.4 device test.
