<p align="right"><strong>English</strong> · <a href="AGENTS.zh_CN.md">简体中文</a></p>

# Repository Guidelines

- This is a Sunflower Buddy V1.4 hardware-reference project, not production firmware.
- Use ESP-IDF 5.5.3 and target only ESP32-S3.
- Start with `git status --short --branch` and preserve unrelated work.
- Hardware constants belong only in `components/bsp/include/bsp_pins.h`.
- Reusable hardware APIs and implementations belong in `components/bsp`.
- Small runnable examples and application behavior belong in `main/demo_*`.
- Wi-Fi/BLUFI provisioning stays in `main/demo_blufi.*`; never log credential
  contents, and reset an active BLUFI connection before starting a new session.
- Do not add cloud protocols, embedded credentials, OTA, product state machines, or media bundles.
- Button and interrupt callbacks must remain non-blocking.
- Reuse the BSP-owned I2C bus; never create a second owner for I2C0.
- Preserve the documented GX8002 voltage values, rail order, and 10 ms delay.
- Report build, host tests, device tests, and remaining hardware checks separately.

Read `docs/hardware-design/SUNFLOWER_BUDDY_V14_HARDWARE_GUIDE.md` before hardware
changes and `docs/development/engineering/blufi-provisioning.md` before
Wi-Fi/BLE provisioning changes.
