<p align="right"><a href="README.md">English</a> · <strong>简体中文</strong></p>

# Sunflower Buddy V1.4 BSP

`include/bsp_pins.h` 是引脚、地址、时钟和供电参数的唯一事实来源，其余公共头文件
分别提供一种硬件能力。

每个驱动负责自身资源：所有 I2C 设备复用 `bsp_i2c_bus()`，回调不得阻塞，音频操作
必须在回调外执行。GX8002 上电保持 IO → 10 ms → CORE，断电保持 CORE → 10 ms → IO。
Wi-Fi 与 BLUFI 是 `main/demo_blufi.*` 中的应用行为，不持有板级专用引脚，因此不属于
BSP API。

## 模块

| 头文件 | 职责 | 调用上下文 |
| --- | --- | --- |
| `bsp_i2c.h` | 持有 I2C0、暴露总线句柄、扫描设备 | 最先初始化；扫描会阻塞。 |
| `bsp_power.hpp` | AXP2101 状态/事件、充电、GX8002 电源轨、休眠/关机 | 事件回调位于 BSP 工作任务；休眠/关机会改变状态。 |
| `bsp_rtc.hpp` | BM8563 时间、闹钟、中断和唤醒来源判断 | 闹钟回调位于 BSP 工作任务。 |
| `bsp_audio.h` | ES8311/I2S 初始化、格式、PCM、增益和音量 | PCM 会阻塞；休眠前调用 `bsp_audio_close()`。 |
| `bsp_button.h` | 四键按下、释放、点击和长按事件 | 回调位于 button 组件任务，必须快速返回。 |
| `bsp_led.h` | RGB、关闭、白灯闪烁 | 白灯闪烁按 `on_ms` 阻塞。 |
| `bsp_motor.hpp` | MX6115 模式、速度、启停、单次动作和 ADC 过流监测 | OCP 回调位于监测任务；触发后排队停止电机。 |
| `bsp_wakeup.hpp` | GX8002 供电/初始化、启停、中断、版本和升级 | 在 PMU 后初始化；回调不得阻塞。 |
| `bsp_modem.h` | LTE UART、波特率和字节收发 | 显式启用，因为 UART0 路由可能影响 console。 |
| `bsp_sleep.h` | EXT1 唤醒配置和协同深睡 | 成功进入深睡后不返回。 |

## 安全初始化顺序

```text
bsp_i2c_init
  ├─ bsp_power_init
  │    └─ bsp_wakeup_init
  ├─ bsp_rtc_init
  └─ bsp_audio_init → bsp_audio_open
bsp_led_init
bsp_motor_init → 可选过流监测
bsp_button_init
可选 bsp_modem_uart_init
```

Demo 在需要 GX8002 恢复路径前挂载 LittleFS。升级必须显式调用，`bsp_wakeup_init()`
不会每次开机重刷 GX8002。
完整应用顺序为 LittleFS → Station Wi-Fi/BLUFI → 共享 I2C →
PMU/RTC/灯/电机/音频/GX8002 → 事件队列与按键。Wi-Fi 先于 I2C 启动不会改变 I2C
资源所有权。

## 初始化与生命周期

- I2C、灯、电机、PMU、RTC、音频和 GX8002 完成初始化后，再次初始化会返回成功。
- `bsp_audio_open()` 可用相同格式重复调用；格式变化时会关闭并重新打开输入输出。
- `bsp_button_init()` 会创建四个按键设备，只应调用一次。
- 过流监测是常驻单任务；再次启用会保留原阈值与回调。
- BSP 没有通用反初始化流程。显式休眠/关机和模组串口路由具有系统级副作用，必须
  单独做真机测试。

## 错误约定

依赖初始化的函数若调用过早返回 `ESP_ERR_INVALID_STATE`；无效范围或空参数按接口返回
`ESP_ERR_INVALID_ARG`。`bsp_motor_*` 包装层会把上游驱动的 FreeRTOS 队列结果规范化为
`ESP_OK`/`ESP_FAIL`。

## 最小使用模式

一次读取完整 PMU 快照，避免组合不同时刻的数据：

```cpp
bsp_power_status_t status{};
ESP_ERROR_CHECK(bsp_power_get_status(&status));
ESP_LOGI("app", "battery=%d%% voltage=%umV charging=%d",
         status.battery_percent, status.battery_mv, status.charging);
```

PCM 传输前必须初始化并打开音频格式：

```cpp
ESP_ERROR_CHECK(bsp_audio_init());
ESP_ERROR_CHECK(bsp_audio_open(BSP_AUDIO_RATE, 16, 1));
ESP_ERROR_CHECK(bsp_audio_set_volume(60));
ESP_ERROR_CHECK(bsp_audio_read(buffer, bytes));
ESP_ERROR_CHECK(bsp_audio_write(buffer, bytes));
```

有意配置 RTC 唤醒；`bsp_sleep_enter()` 成功后进入深睡，不会继续执行：

```cpp
ESP_ERROR_CHECK(bsp_rtc_init());
ESP_ERROR_CHECK(bsp_rtc_set_alarm_after(60));
ESP_ERROR_CHECK(bsp_sleep_enter(bsp_sleep_default_wakeup_mask()));
```

GX8002 恢复和 LTE UART 都属于显式操作：

```cpp
ESP_ERROR_CHECK(bsp_wakeup_upgrade("/littlefs/gx_bl/grus-i2c.boot",
                                   "/littlefs/gx_bl/mcu_nor.bin"));
ESP_ERROR_CHECK(bsp_modem_uart_init(BSP_MODEM_INITIAL_BAUD));
```

这些代码只说明 API，不是 Demo 自动行为。产品使用前必须检查返回值并完成对应真机验收。

引脚、供电时序、排障和真机验收见
[硬件指南](../../docs/hardware-design/SUNFLOWER_BUDDY_V14_HARDWARE_GUIDE.zh_CN.md)。
