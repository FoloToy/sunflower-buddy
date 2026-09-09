<p align="right"><a href="README.md">English</a> · <strong>简体中文</strong></p>

# Sunflower Buddy

这是面向 FoloToy Sunflower Buddy V1.4 的 ESP-IDF 硬件参考工程，专门用于给 AI 编程助手
提供清晰、精简的开发基线，只包含稳定的板级接口、基础驱动实现和可运行 Demo。

本仓库不是完整产品固件。MQTT、OTA、产品状态机、内置账号密钥、媒体资源及云端协议均
被有意排除。

## 工程结构

```text
components/bsp/include/  稳定硬件 API 与唯一引脚定义
components/bsp/src/      Sunflower Buddy V1.4 完整板级硬件适配
main/                    按键/音频 Demo 与 BLUFI 配网行为
docs/                    硬件事实与开发说明
tests/                   仓库结构测试
tools/                   本地及 CI 验证入口
```

可复用硬件代码位于 `components/bsp`，应用行为位于 `main`。

## 硬件基线

ESP32-S3 配置 16 MB Flash 和 8 MB Octal PSRAM，连接 ES8311 音频、四个按键、
一颗 RGB 灯、MX6115 电机、AXP2101 电源管理、BM8563 RTC、GX8002 唤醒词芯片和
LTE UART 传输。扩展 Demo 前先阅读[能力契约](docs/README.zh_CN.md)和
[BSP API 总览](components/bsp/README.zh_CN.md)。

## Demo 操作

- 按住对话键录音，松开后立即回放录音，电机在回放期间持续转动。
- GX8002 识别到唤醒词：电机以最高速度转动 500 ms。
- 单击 Wi-Fi 键：输出板级/Wi-Fi 状态，并执行白灯和电机自检。
- 长按 Wi-Fi 键 1 秒：清除已保存的 Wi-Fi 凭据并重新开启 BLUFI 配网；已有 BLUFI
  客户端会断开，必须重新连接。
- 音量加/减键：每次将回放音量增加/减少 10%，范围为 0%–100%。

单一 Demo 会使用麦克风、扬声器、RGB 灯、电机、按键、PMU、GX8002、Wi-Fi 和
BLE。单次录音最长 15 秒，录音缓冲区位于 PSRAM，执行结果通过串口日志输出。

## BLUFI 配网

Demo 启动时会自动使用 ESP-IDF Wi-Fi 驱动保存的 Station 配置连接 2.4 GHz Wi-Fi；
断线后按 1–30 秒指数退避持续重连。没有保存配置时，设备以
`SunflowerBuddy-XXXX` 名称广播 BLUFI，`XXXX` 来自蓝牙 MAC 地址。使用兼容
Espressif BLUFI 的 App 可扫描热点并下发凭据。

BLUFI 传输采用 ESP-IDF 示例的 DH/AES 协商，并限制 GATT MTU 以适配 BLUFI 单字节
帧长度；日志不会输出密码内容。Wi-Fi 成功后，客户端一旦断开就不会重新广播，但当前
会保留已经连接的 GATT 客户端，也没有 BLE 自动关闭定时器。生命周期、安全边界、
日志、重置流程和验收方法见 [BLUFI 配网指南](docs/development/engineering/blufi-provisioning.zh_CN.md)。
该功能只完成本地 Wi-Fi 配网，不增加任何云协议。

除 Demo 直接使用的能力外，BSP 还保留 AXP2101 电压/电量/充电状态、电源来源、
PMU 中断、充电开关、休眠和关机，BM8563 时间/闹钟，四键完整动作，
MX6115 模式/速度/过流保护，ES8311 生命周期，LTE UART 和深睡唤醒 API。关机和
深睡等会改变设备状态的接口不会被 Demo 自动调用。

Sunflower 2 的 GX8002 唤醒词固件会打包进 `storage` LittleFS 分区，并挂载到
`/littlefs/gx_bl`。需要恢复或升级时可显式调用 `bsp_wakeup_upgrade()`；Demo 不会在
每次开机时自动重刷 GX8002。

## 编译

```bash
source /path/to/esp-idf-v5.5.3/export.sh
# 新 checkout 或 target 变化时执行一次。
idf.py set-target esp32s3
idf.py build
```

运行仓库门禁：

```bash
./tools/validate.sh --static
./tools/validate.sh --firmware
```

连接开发设备后烧录和监视：

```bash
idf.py -p <port> flash monitor
```

从[文档索引](docs/README.zh_CN.md)开始阅读，其中链接了环境搭建、代码规范、硬件约束、
排障和真机验收。贡献者与 AI 编程助手还必须遵守 [AGENTS.zh_CN.md](AGENTS.zh_CN.md)。

实际硬件行为仍需在 Sunflower Buddy V1.4 设备上验证。
