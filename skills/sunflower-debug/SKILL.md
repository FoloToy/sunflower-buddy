---
name: sunflower-debug
description: Investigate board faults without importing product logic for sunflower-buddy.
---

[简体中文](SKILL.zh_CN.md)

# Investigate board faults without importing product logic

Read docs/BSP.md and affected public API/implementation. Confirm board profile, power source, chip, Flash and PSRAM. Compare observed I2C addresses and audio pin directions against bsp_pins.h. Isolate one peripheral with the independent demo. Capture sanitized logs and propagate errors. Keep GPIO callbacks non-blocking and prevent duplicate bus/lifecycle owners. Reproduce before changing hardware constants; do not guess values from another board.
