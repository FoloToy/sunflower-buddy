<p align="right"><strong>English</strong> · <a href="README.zh_CN.md">简体中文</a></p>

# Sunflower Buddy V1.4 BSP

`include/bsp_pins.h` is the only source of board pins, addresses, clock rates,
and power parameters. Each other public header exposes one hardware capability.

Drivers own their resources: all I2C devices reuse `bsp_i2c_bus()`, callbacks do
not block, audio work runs outside callbacks, and the GX8002 rail sequence stays
IO → 10 ms → CORE when powering up and CORE → 10 ms → IO when powering down.
Wi-Fi and BLUFI are application behavior in `main/demo_blufi.*`; they do not own
board-specific pins and intentionally are not BSP APIs.

## Modules

| Header | Responsibility | Call-context notes |
| --- | --- | --- |
| `bsp_i2c.h` | Own I2C0, expose its bus handle, scan devices | Initialize first; scanning blocks. |
| `bsp_power.hpp` | AXP2101 status/event mask, charging, GX8002 rails, sleep/shutdown | Event callback is a BSP worker task; sleep/shutdown are state-changing. |
| `bsp_rtc.hpp` | BM8563 time, alarms, IRQ and wake-source check | Alarm callback is a BSP worker task. |
| `bsp_audio.h` | ES8311/I2S initialization, format, PCM I/O, gain and volume | PCM I/O blocks; call `bsp_audio_close()` before sleep. |
| `bsp_button.h` | Four keys with press, release, click and long-press events | Callback runs in the button component task and must return quickly. |
| `bsp_led.h` | RGB output, clear, white flash | White flash blocks for `on_ms`. |
| `bsp_motor.hpp` | MX6115 mode, speed, start/stop/pulse and ADC overcurrent monitor | OCP callback runs in its monitor task; a trip queues motor stop. |
| `bsp_wakeup.hpp` | GX8002 power/init, enable, IRQ, version and upgrade | Initialize after PMU; callback must not block. |
| `bsp_modem.h` | LTE UART setup, baud change and byte I/O | Explicit opt-in because UART0 routing can affect console output. |
| `bsp_sleep.h` | EXT1 wake configuration and coordinated deep sleep | Does not return after successful deep-sleep entry. |

## Safe initialization order

```text
bsp_i2c_init
  ├─ bsp_power_init
  │    └─ bsp_wakeup_init
  ├─ bsp_rtc_init
  └─ bsp_audio_init → bsp_audio_open
bsp_led_init
bsp_motor_init → optional overcurrent monitor
bsp_button_init
optional bsp_modem_uart_init
```

The demo mounts LittleFS before GX8002 recovery paths are used. Firmware upgrade
is explicit; `bsp_wakeup_init()` does not rewrite the GX8002 on every boot.
Its complete application order is LittleFS → station Wi-Fi/BLUFI → shared I2C →
PMU/RTC/LED/motor/audio/GX8002 → event queue and buttons. Starting Wi-Fi before
I2C does not change I2C ownership.

## Initialization and lifetime

- I2C, LED, motor, PMU, RTC, audio, and GX8002 initialization return success
  when their completed initialization state is already active.
- `bsp_audio_open()` may be called again with the same format; a different
  format closes and reopens both codec directions.
- `bsp_button_init()` creates four button devices and is intended to run once.
- The overcurrent monitor is a single persistent task; a second enable call
  keeps the existing threshold and callback.
- The BSP has no general teardown sequence. Explicit sleep/shutdown and modem
  routing have system-wide side effects and require dedicated device tests.

## Error contract

Functions that require initialization return `ESP_ERR_INVALID_STATE` when called
too early. Invalid ranges and null arguments return `ESP_ERR_INVALID_ARG` where
applicable. `bsp_motor_*` wrappers normalize the upstream driver's FreeRTOS
queue result to `ESP_OK`/`ESP_FAIL`.

## Minimal usage patterns

Read a complete PMU snapshot rather than combining values from different times:

```cpp
bsp_power_status_t status{};
ESP_ERROR_CHECK(bsp_power_get_status(&status));
ESP_LOGI("app", "battery=%d%% voltage=%umV charging=%d",
         status.battery_percent, status.battery_mv, status.charging);
```

Audio must be initialized and opened before PCM transfer:

```cpp
ESP_ERROR_CHECK(bsp_audio_init());
ESP_ERROR_CHECK(bsp_audio_open(BSP_AUDIO_RATE, 16, 1));
ESP_ERROR_CHECK(bsp_audio_set_volume(60));
ESP_ERROR_CHECK(bsp_audio_read(buffer, bytes));
ESP_ERROR_CHECK(bsp_audio_write(buffer, bytes));
```

Configure an RTC wake deliberately; this flow enters deep sleep and does not
continue after `bsp_sleep_enter()` succeeds:

```cpp
ESP_ERROR_CHECK(bsp_rtc_init());
ESP_ERROR_CHECK(bsp_rtc_set_alarm_after(60));
ESP_ERROR_CHECK(bsp_sleep_enter(bsp_sleep_default_wakeup_mask()));
```

GX8002 recovery and LTE UART are opt-in operations:

```cpp
ESP_ERROR_CHECK(bsp_wakeup_upgrade("/littlefs/gx_bl/grus-i2c.boot",
                                   "/littlefs/gx_bl/mcu_nor.bin"));
ESP_ERROR_CHECK(bsp_modem_uart_init(BSP_MODEM_INITIAL_BAUD));
```

The snippets are API examples, not automatic demo behavior. Check every return
value and complete the relevant device-acceptance items before product use.

See the [hardware guide](../../docs/hardware-design/SUNFLOWER_BUDDY_V14_HARDWARE_GUIDE.md)
for pin usage, power sequencing, troubleshooting, and device acceptance.
