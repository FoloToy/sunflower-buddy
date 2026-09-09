<p align="right"><strong>English</strong> · <a href="SUNFLOWER_BUDDY_V14_HARDWARE_GUIDE.zh_CN.md">简体中文</a></p>

# Sunflower Buddy V1.4 Hardware Guide

Source-of-truth priority: measured/product specification, then
`components/bsp/include/bsp_pins.h`, then the BSP adapter, then this document.

## Platform

| Item | Configuration |
| --- | --- |
| MCU | ESP32-S3, 16 MB Flash, 8 MB octal PSRAM |
| Radio reference | 2.4 GHz Wi-Fi station and BLE 4.2 BLUFI |
| Audio | ES8311, 24 kHz input/output; ESP TX GPIO3, RX GPIO9; NS4150 enable GPIO15 |
| Power | AXP2101 at I2C address `0x34`, interrupt GPIO2 |
| Motor | MX6115 PWM GPIO47/GPIO38, over-current input GPIO14 |
| LED | addressable RGB data GPIO6 |
| LTE UART | physical TX GPIO4, RX GPIO5; legacy adapter intentionally swaps DTE RX/TX assignment |
| Shared I2C0 | SDA GPIO41, SCL GPIO42 |
| RTC | BM8563 at `0x51`, interrupt GPIO1 |
| Wake processor | GX8002 interrupt GPIO13 |

Buttons are active-low: mode GPIO21, voice GPIO12, volume-up GPIO7, and
volume-down GPIO16. The deep-sleep wake mask includes these inputs plus the PMU,
GX8002, and RTC lines defined by the BSP.

## Invariants

- GX8002 VDDIO must be configured to 3.3 V and enabled before its 1.0 V core;
  shutdown reverses that order with the documented delay.
- Stop codec input/output before PMU sleep so ES8311 cannot hold the I2C bus.
- Audio volume is expressed as 0–100 percent and is applied after codec creation.
- LTE uses 115200 as the known startup baud and 921600 as the operating target.
- Wi-Fi provisioning targets 2.4 GHz station networks; do not document 5 GHz,
  SoftAP, BLE SMP, bonding, or encrypted NVS as implemented behavior.
- The board has no display; do not expose LCD/LVGL capabilities.

## Resource ownership and initialization

I2C0 is initialized once by `bsp_i2c_init()`. AXP2101, BM8563, GX8002, and
ES8311 reuse that bus; a healthy demo scan normally reports addresses `0x18`,
`0x2F`, `0x34`, and `0x51`. A missing address is a device/power/bus fault, not a
reason to create another bus instance.

Initialize the PMU before GX8002 because the PMU controls its rails. Initialize
the codec after I2C and open the PCM format before reads or writes. LED, motor,
and key drivers own their GPIO/RMT/LEDC/button resources. LTE UART initialization
is opt-in because it routes UART0 to GPIO5/GPIO4 and may change console behavior.

## Peripheral behavior

### AXP2101 power

The BSP exposes battery percentage and voltage, system/VBUS voltage, battery and
VBUS presence, charging state, power-on/off sources, charging enable, PMU events,
sleep, and shutdown. PMU GPIO2 only wakes a worker task; I2C register reads and
the application callback happen outside the ISR. The event mask includes battery
warnings, insertion/removal, charge start/done, temperature faults, overcurrent,
charge timeout, and power-key activity.

`bsp_power_shutdown()` and `bsp_power_enter_sleep()` change hardware state. They
are deliberately not bound to demo keys. `bsp_power_driver()` exists for an
advanced board experiment that needs an AXP2101 operation not wrapped by the
stable API; code using it is coupled to the managed driver version.

### Audio

I2S0 uses BCLK GPIO10, WS GPIO46, MCLK GPIO11, ESP TX GPIO3, and ESP RX GPIO9.
The validated codec setup is slave mode with `use_mclk=false`; the ES8311 derives
its clock from BCLK. The demo opens 24 kHz, 16-bit, mono PCM, input gain 30 dB,
and output volume 60%. A maximum 15-second recording consumes 720,000 bytes in
PSRAM. Red means recording; green means playback.

### Buttons and demo flow

All four keys are active-low. The BSP reports press, release, click, and
long-press events. The demo queues these callbacks into one application task:

- hold voice to record and release to play the captured PCM;
- volume keys change output by 10%, clamped to 0–100%;
- clicking Wi-Fi/mode prints status, flashes white for 250 ms, and pulses the motor;
- holding Wi-Fi/mode for 1 second queues a saved-credential reset and restarts
  BLUFI provisioning;
- a GX8002 event pulses the motor at duty 1023 for 500 ms.

The button callback only queues the long-press event. Wi-Fi disconnect, NVS
updates, and BLUFI advertising are performed by application tasks.

### BLUFI Wi-Fi provisioning

The demo runs ESP32-S3 station Wi-Fi and BLE-only BLUFI. ESP-IDF stores station
configuration in NVS; the firmware contains no SSID or password and never logs
the received password. BLUFI uses DH key negotiation, AES-CFB encryption, and a
CRC checksum based on the ESP-IDF 5.5.3 example.

At boot, a saved SSID triggers an automatic connection attempt. Every later
disconnect is retried indefinitely with exponential delays of 1, 2, 4, 8, 16,
and at most 30 seconds. A successful DHCP lease resets the delay. Without saved
credentials, the BLE name is `SunflowerBuddy-XXXX`, using the last two bytes of
the Bluetooth MAC address, and BLUFI advertising starts automatically. A
one-second Wi-Fi/mode hold clears only the Wi-Fi driver's persistent settings
and returns to provisioning; it does not erase unrelated NVS data. If a BLUFI
client is connected, reset closes that connection so packet sequence numbers
and security negotiation restart cleanly; reconnect to the advertised device.

After DHCP succeeds, provisioning becomes idle. Advertising stays off after the
client disconnects, but the current reference retains an already-connected GATT
client and keeps Bluedroid/the BLE controller initialized. It has no automatic
post-success BLE timeout. See the
[BLUFI provisioning guide](../development/engineering/blufi-provisioning.md)
for the exact state table, security boundary, logs, and repeated-session tests.

BLUFI provides local station provisioning only. There is no SoftAP mode,
application protocol, remote service, OTA, or cloud connection in this demo.

### Motor and overcurrent input

MX6115 uses GPIO47/GPIO38 with 20 kHz, 10-bit LEDC PWM. Public duty values are
0–1023. Playback starts continuous forward movement at 1023 and queues stop when
PCM output ends. GPIO14 maps to ESP32-S3 ADC2 channel 3. The monitor samples every
20 ms, applies a 0.9/0.1 exponential moving average, requires 100 ms above the
default raw threshold 2500, stops the motor, and resets after falling 200 counts
below the threshold. These values are a baseline, not a calibrated current in
milliamps.

### GX8002

The LittleFS `storage` image contains `gx_bl/grus-i2c.boot` and
`gx_bl/mcu_nor.bin`, mounted by the demo under `/littlefs/gx_bl`. Initialization
queries the existing firmware and installs the active-low GPIO13 callback; it
does not flash firmware. Recovery is explicit:

```cpp
bsp_wakeup_upgrade("/littlefs/gx_bl/grus-i2c.boot",
                   "/littlefs/gx_bl/mcu_nor.bin");
```

Do not interrupt power during an upgrade. Validate the firmware checksum and
version on the device afterward.

### RTC and deep sleep

BM8563 supports wall-clock setting, calendar alarms, relative timer alarms,
alarm clearing, and RTC wake detection. The default EXT1 ANY_LOW wake mask
contains four keys, PMU GPIO2, GX8002 GPIO13, and RTC GPIO1. `bsp_sleep_enter()`
configures the mask, closes both codec directions, asks AXP2101 to enter its board
sleep state, and starts ESP deep sleep.

### LTE UART

The BSP provides transport only: UART setup, baud changes, flush, and byte I/O.
It does not implement AT command parsing, baud negotiation, PPP, networking, or
cloud protocols. The known module startup rate is 115200 and the desired runtime
rate is 921600. A higher-level implementation must obtain an `OK` response at
the current rate before changing either endpoint and must handle the module
already being in PPP data mode.

## Flash and memory layout

| Offset | Size | Purpose |
| --- | --- | --- |
| `0x9000` | 24 KB | NVS |
| `0xF000` | 4 KB | PHY init data |
| `0x10000` | 4 MB | Factory application |
| `0x410000` | 64 KB | Core dump |
| `0x420000` | 512 KB | LittleFS GX8002 firmware storage |

The defined partitions end at `0x4A0000`; the remainder of the 16 MB Flash is
intentionally unallocated by this reference layout. Do not invent a use for it
without changing and reviewing `partitions.csv`.

The demo depends on octal PSRAM. Allocation failure is handled as a failed
recording attempt rather than falling back to a large internal-RAM buffer.

## Troubleshooting

| Symptom | Checks |
| --- | --- |
| Recorded peak is zero / playback is silent | Confirm scan address `0x18`, GPIO3 TX/GPIO9 RX direction, codec open result, microphone power, and I2S clock activity. |
| Volume changes but remains quiet | Confirm the value reaches `bsp_audio_set_volume()` as 0–100 and is not scaled twice; verify PA GPIO15 and speaker hardware. |
| Motor API logs an error despite movement | Use BSP wrappers; the upstream queue API reports `pdPASS` as integer 1 and must be normalized. |
| Motor stops unexpectedly | Capture raw/filtered GPIO14 values and calibrate the OCP threshold under known load. |
| GX8002 is missing at `0x2F` | Verify AXP2101 first, then the 3.3 V I/O → 10 ms → 1.0 V core sequence. |
| RTC alarm repeats | Clear the BM8563 alarm flag in task context and confirm GPIO1 returns inactive. |
| Modem returns binary bytes to `AT` | It is likely in PPP data mode; use the module's guarded escape procedure before command mode. |
| Deep sleep immediately wakes | Read EXT1 wake status and find which active-low source was already asserted before sleep. |
| `SunflowerBuddy-XXXX` is not visible | Confirm BLE/BLUFI initialization, verify no saved SSID is active, or hold Wi-Fi/mode for one second to restart provisioning. |
| Wi-Fi repeatedly disconnects | Read the disconnect reason in the serial log, verify the AP credentials and 2.4 GHz signal, and observe the 1–30 second retry backoff. |
| Repeated BLUFI reports `seq 0 is not expect N` | The phone reused an old GATT session. Hold Wi-Fi, wait for the forced BLE disconnect, reconnect, and start a new provisioning attempt. |
| BLUFI scan reports `ESP_ERR_WIFI_NOT_STARTED` | Confirm current firmware: reset must clear station config without restoring/stopping the whole Wi-Fi stack. |
| `gpio_install_isr_service... already installed` appears | PMU and RTC share ESP-IDF's global GPIO ISR service. If both BSP initializers return success and their interrupts work, the second install attempt is informational; do not install another service. |
| `Incorrect size of core dump image` appears at boot | The coredump partition contains stale or non-coredump bytes. This is not proof of a current crash; preserve it for investigation or deliberately erase only that partition. |

## Device acceptance

Record the board revision, firmware commit, power source, battery state, and test
conditions. At minimum verify:

- the four expected I2C addresses appear and repeated access is stable;
- each key produces press/release/click/long-press without blocking the task;
- recorded speech has a nonzero peak and audible, undistorted playback;
- volume reaches both limits in ten presses from the opposite limit;
- RGB colors and white flash match the requested duration;
- motor start/continuous stop/pulse/direction work and OCP stops a controlled load;
- PMU status and insertion/charge/power-key events match physical changes;
- RTC time, alarm interrupt, alarm clear, and RTC deep-sleep wake work;
- GX8002 version, enable/disable, wake interrupt, rail cycling, and explicit
  recovery upgrade work;
- modem command mode works at the expected baud and UART0/console coexistence is
  understood;
- every default EXT1 source can wake the device and no inactive source causes a
  wake loop;
- a clean device advertises `SunflowerBuddy-XXXX`, accepts BLUFI
  credentials, obtains an IP address, and stops advertising after the client
  disconnects;
- a Wi-Fi loss triggers automatic backoff/reconnect, a reboot reconnects from
  NVS, and a one-second Wi-Fi/mode hold clears credentials and resumes BLUFI.
- three consecutive reset/reprovision cycles close the old GATT session and do
  not produce frame-length, sequence, or stopped-Wi-Fi errors.

Destructive PMU shutdown, charging disable, deep sleep, GX8002 upgrade, motor
stall/OCP, and modem baud changes require an intentional device test. Compilation
alone is not physical validation.
