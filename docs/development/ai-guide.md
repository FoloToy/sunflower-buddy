<p align="right"><strong>English</strong> · <a href="ai-guide.zh_CN.md">简体中文</a></p>

# AI Development Guide

Start with `AGENTS.md`, then read only the public BSP header and implementation
for the capability being changed.

```text
requirement
  └─ main/main.cpp event routing and initialization
      ├─ main/demo_record_playback.cpp audio/feedback example
      ├─ main/demo_blufi.*         Wi-Fi/BLE provisioning behavior
      └─ components/bsp/include/   stable board API
          └─ components/bsp/src/   hardware implementation
              └─ bsp_pins.h        hardware source of truth
```

Add reusable hardware behavior to the BSP and demonstrate it in a focused
`main/demo_*` module. Keep event routing in `main.cpp`; keep Wi-Fi/BLUFI state in
`demo_blufi.*`. Slow work must run in an application task, not a button,
Bluetooth, Wi-Fi, timer, or interrupt callback. Reuse the existing I2C bus and
preserve explicit power sequencing. Do not infer pins or electrical behavior
from a generic ESP32-S3 board.

## Before editing

1. Run `git status --short --branch` and preserve existing work.
2. Read the relevant section of the
   [hardware guide](../hardware-design/SUNFLOWER_BUDDY_V14_HARDWARE_GUIDE.md).
3. Trace the public header, implementation, demo call site, component manifest,
   generated configuration defaults, and repository check for the capability.
4. State assumptions that change hardware behavior. Ask before changing a pin,
   voltage, partition boundary, persistent format, or destructive action.

## Implementation boundaries

- Add a BSP function when behavior owns a peripheral, pin, bus, power sequence,
  or reusable board policy. Add demo code when behavior is only user flow.
- ISR handlers only notify a task. Component callbacks only enqueue lightweight
  events. Blocking audio, delays, filesystem I/O, and power transitions belong
  in application or BSP worker tasks.
- BLUFI callbacks validate and enqueue input; Wi-Fi configuration, scanning,
  reconnect, and credential reset run in the provisioning task. Never log a
  password, embed a credential, or describe the example DH flow as production
  identity authentication.
- Keep dangerous capabilities callable but not automatic: the demo must not
  trigger PMU shutdown, deep sleep, charging disable, RTC alarms, modem sessions,
  or GX8002 firmware upgrades without an explicit example requirement.
- Preserve the bundled GX8002 files and verify their checksums after moving or
  changing storage rules.
- Do not add unconfirmed peripherals or production/cloud behavior unless the
  project scope is explicitly changed and the hardware is confirmed.

## Review checklist

- Initialization and failure paths are idempotent where documented.
- Ranges and null pointers are rejected before touching hardware.
- Shared I2C, UART0 routing, LEDC channels, ADC2 use, task stacks, PSRAM, and
  LittleFS ownership do not conflict. Wi-Fi/BLE lifecycle and callback queues
  do not block each other or retain stale BLUFI sequence/security state.
- English and Chinese documents reflect the same final behavior.
- Build, host tests, device tests, and unverified hardware checks are separated.

Before delivery run `./tools/validate.sh --static` and, with ESP-IDF 5.5.3
active, `./tools/validate.sh --firmware`. A successful build is not device
validation; state the remaining on-device checks.
