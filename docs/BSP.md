[简体中文](BSP.zh_CN.md)

# Sunflower Buddy V1.4 / 小葵 2.0

The board is ESP32-S3, 16 MB Flash and 8 MB octal PSRAM. Product name
2.0, BSP board V1.4 and production HW_V1.4.1 are different version fields.
Do not assume full V1.4/V1.4.1 compatibility without hardware checks.
The default main only initializes I2C, PMU and RGB. The independent demo retains
hold-to-record/release-to-play, motor feedback, buttons, wake processor and
BLUFI provisioning. Its `main/data` and storage partition are demo-only.
See the [detailed guide](hardware-design/SUNFLOWER_BUDDY_V14_HARDWARE_GUIDE.md)
for that demo's controls and its distinct Flash layout. The root baseline
uses NVS 0x9000/24KB, PHY 0xF000/4KB, factory 0x10000/0xFF0000.

| API | Hardware / ownership |
| --- | --- |
| bsp_i2c | I2C0 SDA41/SCL42, one shared bus |
| bsp_power | AXP2101 0x34, IRQ2; voltage/charge/events/sleep |
| bsp_audio | ES8311 0x18, I2S0 MCLK11/BCLK10/WS46/TX3/RX9, PA15 |
| bsp_button | Active-low keys 21/12/7/16, non-blocking event callbacks |
| bsp_led | WS2812 data6 |
| bsp_motor | MX6115 47/38, OCP ADC2 GPIO14 |
| bsp_rtc | BM8563 0x51, IRQ1, time/alarm |
| bsp_wakeup | GX8002 0x2F, IRQ13 |
| bsp_modem | UART0 TX5/RX4, transport only |
| bsp_sleep | Explicit opt-in deep sleep, never called by baseline |

## Initialization, ownership and failures

Initialize I2C before PMU. GX8002 rails: core/IO off, IO=3.3V and core=1.0V, then IO → 10ms → core; disable in reverse order. Audio: init → open → read/write; volume is 0–100%; close before PMU sleep.

One BSP owns each bus; one application owner serializes lifecycle calls. Callbacks cannot perform networking/audio/storage; copy events into an application queue. Audio I/O can block. Propagate initialization errors; missing hardware is not success. The baseline starts no Wi-Fi, Bluetooth, cloud service or product state machine.

## Validation and device acceptance

`./tools/build.sh` compiles baseline and demo. Compiler/host results do not prove device behavior. Device acceptance must record physical revision/chip, Flash/PSRAM, pins, bus addresses, rails, nonzero microphone capture/audible playback, keys/touch/NFC where fitted, RGB and RTC/wake events. Motor, haptic, shutdown, sleep, 4G and wake-chip firmware upgrade are intentional opt-in tests.

Report `Build`, `Host tests`, `Device tests` and `Unverified` separately. Installing a development partition layout can overwrite production identity/data; confirm the exact board/image/data impact and obtain authorization before writing.
