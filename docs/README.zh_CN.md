<p align="right"><a href="README.md">English</a> · <strong>简体中文</strong></p>

# 文档

Sunflower Buddy 是一份可执行的 Sunflower Buddy V1.4 硬件参考：稳定板级 API、单一
Demo、硬件约束和可重复验证集中在同一仓库中。它不是产品固件。

## 能力契约

| 能力 | 实现 | 公开入口 | 重要边界 |
| --- | --- | --- | --- |
| 共享 I²C | 一条 400 kHz ESP-IDF I2C0 总线 | `bsp_i2c_*` | 禁止创建第二个 I2C0 所有者。 |
| 音频 | ES8311 全双工 PCM | `bsp_audio_*` | 读写会阻塞；PMU 休眠前关闭 codec。 |
| 按键 | 四个低有效 GPIO 按键 | `bsp_button_*` | 回调不得阻塞。 |
| Wi-Fi 配网 | 仅 BLE 的 BLUFI 与持久化 2.4 GHz Station 配置 | `demo_blufi_*` | 不内置凭据或云协议；成功后没有 BLE 自动关闭定时器；重置操作在回调外执行。 |
| 灯光 | 一颗兼容 WS2812 的 RGB 灯 | `bsp_led_*` | `bsp_led_flash_white()` 会按时长阻塞。 |
| 电机 | MX6115 PWM 与 GPIO14 ADC 过流监测 | `bsp_motor_*` | 占空比 0–1023；阈值需真机标定。 |
| 电源 | AXP2101 状态、事件、充电、休眠和关机 | `bsp_power_*` | 休眠/关机会改变硬件状态，Demo 不触发。 |
| RTC | BM8563 时间、闹钟中断和唤醒判断 | `bsp_rtc_*` | 在共享 I²C 初始化后使用。 |
| 唤醒词 | GX8002 供电、中断、版本和升级 | `bsp_wakeup_*` | 保持电压、供电顺序和 10 ms 延时。 |
| LTE 传输 | UART 初始化和字节收发 | `bsp_modem_uart_*` | UART0 路由可能影响 console；不包含 PPP/云协议。 |
| 深睡 | EXT1 唤醒掩码和 PMU 协同进入 | `bsp_sleep_*` | 默认包含四键、PMU、GX8002 和 RTC。 |

硬件常量只以 [`bsp_pins.h`](../components/bsp/include/bsp_pins.h) 为权威来源，未确认
外设不属于能力契约。

## 文档地图

- [开发规范](development/README.zh_CN.md)：AI 工作流、环境、代码规范、构建、测试、
  烧录和结果报告。
- [BLUFI 配网](development/engineering/blufi-provisioning.zh_CN.md)：生命周期、安全边界、
  手机流程、日志、排障和真机验收。
- [贡献规范](contribution/README.zh_CN.md)：commit/PR 与双语文档约定。
- [硬件设计](hardware-design/README.zh_CN.md)：规格、硬件指南、约束、排障和真机验收。
- [BSP API 总览](../components/bsp/README.zh_CN.md)：初始化顺序与模块级 API。
- [变更记录](CHANGELOG.zh_CN.md)：用户可见基线变化。

## 仓库结构

```text
components/bsp/include/  稳定 API 与权威引脚定义
components/bsp/src/      板级资源所有权与硬件实现
main/                    单一录音回放与 BLUFI 参考 Demo
main/data/gx_bl/         保留的 GX8002 boot 与唤醒词固件
docs/                    双语工程和硬件文档
tests/                   仓库契约主机测试
tools/                   本地静态与隔离固件验证
```
