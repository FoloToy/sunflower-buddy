---
name: sunflower-device-test
description: Validate firmware on an explicitly authorized device for sunflower-buddy.
---

[简体中文](SKILL.zh_CN.md)

# Validate firmware on an explicitly authorized device

Read docs/BSP.md. Discover ports read-only; do not open/reset arbitrary devices. Confirm board revision, exact image and partition/data impact with the user before writing. Never erase identity partitions by default. Verify power, bus addresses, controls, audio input/output and fitted peripherals. Record serial evidence and human-observed outcomes separately. Shutdown, motor/haptic, sleep, cellular and wake-chip firmware writes are intentional tests. Do not label tests PASS if not actually observed.
