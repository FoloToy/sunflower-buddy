<p align="right"><a href="specifications.md">English</a> · <strong>简体中文</strong></p>

# 规格

本文只列出当前板级适配已确认的能力，不根据 ESP32-S3 或各外设芯片的理论能力扩大
产品声明。

| 项目 | Sunflower Buddy V1.4 基线 |
| --- | --- |
| 软件基线 | ESP-IDF 5.5.3，target 固定为 ESP32-S3 |
| MCU 与内存 | ESP32-S3、16 MB Flash、8 MB Octal PSRAM |
| 无线参考 | 2.4 GHz Wi-Fi Station 与 BLE 4.2 BLUFI；已保存配置自动连接，断线按 1–30 秒退避重连 |
| 音频 | ES8311 codec、麦克风输入、扬声器输出，Demo 使用 24 kHz 单声道 |
| 输入 | 四个低有效按键：Wi-Fi/模式、对话、音量加、音量减 |
| 反馈 | 一颗可寻址 RGB 灯和 MX6115 驱动电机 |
| 电源 | AXP2101 PMIC，提供电池状态、充电事件/控制、休眠和关机 |
| 时钟 | BM8563 RTC，支持闹钟中断和深睡唤醒 |
| 唤醒词 | GX8002，具备独立电源轨、中断、版本查询、启停和固件升级 |
| 蜂窝链路 | LTE 模组 UART；已知启动波特率 115200，目标波特率 921600 |
| 共享控制总线 | I2C0 400 kHz，AXP2101、BM8563、GX8002 和 ES8311 共用 |
| 存储布局 | 24 KB NVS、4 KB PHY 数据、4 MB factory app、64 KB coredump、512 KB LittleFS；其余 Flash 未分区 |
| Demo | 按住录音/松开回放、BLUFI 配网/重置、10% 音量步进、灯光/电机反馈和状态日志 |

本仓库明确不包含产品/云状态机、OTA、内置凭据和未确认外设。BLUFI 示例加密不代表
产品级身份认证、BLE SMP 或加密 NVS 支持。
