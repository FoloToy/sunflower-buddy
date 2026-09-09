<p align="right"><strong>English</strong> · <a href="coding-conventions.zh_CN.md">简体中文</a></p>

# Coding Conventions

## Placement and naming

- Follow adjacent C/C++ style with four-space indentation. Public board APIs use
  `bsp_`; file-local state uses `s_`; hardware constants use `BSP_*`.
- Put reusable hardware ownership and sequencing in `components/bsp`. Keep demo
  state and user interaction in `main`; Wi-Fi/BLUFI provisioning remains in
  `main/demo_blufi.*` because it is not a board-specific peripheral API.
- Define pins, bus instances, addresses, clock rates, and validated electrical
  parameters only in `components/bsp/include/bsp_pins.h`.
- Do not add production services, credentials, OTA, cloud protocols, or product
  state machines to this hardware-reference repository.

## Ownership and concurrency

- Initialize I2C once through `bsp_i2c_init()` and reuse `bsp_i2c_bus()`.
- Button and GX8002 callbacks run in component-task context and must return
  quickly. PMU and RTC callbacks run in BSP worker tasks. Queue application work
  instead of recording, playing, sleeping, or waiting in callbacks.
- Wi-Fi, IP, BLE, and timer callbacks must only copy bounded data, update atomic
  state, or enqueue work. Scans, NVS changes, reconnect policy, and reset flows
  belong in the provisioning task.
- PCM reads/writes and LED flash helpers block. Call them from an application
  task, never an ISR.
- Keep buffers owned by the task that uses them. The demo allocates its recording
  buffer in PSRAM and frees it when playback ends.
- Check every `esp_err_t`. Treat the upstream MX6115 queue result through the BSP
  wrappers; do not interpret its raw `pdPASS` value as an ESP-IDF error.
- Never log Wi-Fi password contents. Logs may include bounded credential lengths
  and numeric disconnect reasons; redact SSIDs, BSSIDs, IPs, and device addresses
  before publishing device logs.

## Hardware rules

- Preserve GX8002 power-up order: 3.3 V I/O, 10 ms, then 1.0 V core. Power-down
  reverses that order.
- Close ES8311 input/output before putting AXP2101 and the MCU into deep sleep.
- Motor duty is 0–1023. Keep overcurrent monitoring enabled for unattended or
  continuous movement and validate thresholds on real hardware.
- Audio volume is 0–100. Application controls change it in 10% steps; drivers
  must not silently rescale stored percentages.
- UART0 is routed to the LTE modem when `bsp_modem_uart_init()` is called. Account
  for the active console configuration before using it.

## Tests and documentation

- Update host checks when changing repository contracts or pure logic.
- Hardware changes require device acceptance results; compilation alone is not
  proof of electrical, acoustic, RF, power, or timing behavior.
- Provisioning changes require cleared/saved station configuration, retry,
  long-press reset, and at least three repeated BLUFI session tests with the app
  version recorded.
- Keep English and Simplified Chinese documentation paired and factually aligned.
- Report build, host tests, device tests, and unverified items separately.
