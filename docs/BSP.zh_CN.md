[English](BSP.md)

# Sunflower Buddy V1.4 / 小葵 2.0

开发板为 ESP32-S3、16MB Flash、8MB 八线 PSRAM。产品名 2.0、BSP 板卡 V1.4、量产 HW_V1.4.1 是不同版本字段，不能自动假定逐项兼容。
默认 main 只初始化 I2C、PMU 与 RGB。独立 Demo 保留按住录音/松开回放、电机反馈、按键、唤醒芯片与 BLUFI 配网；其 main/data 和 storage 分区仅供 Demo 使用。
[详细指南](hardware-design/SUNFLOWER_BUDDY_V14_HARDWARE_GUIDE.zh_CN.md) 描述 Demo 操作及其独立 Flash 布局。根工程为 NVS 0x9000/24KB、PHY 0xF000/4KB、factory 0x10000/0xFF0000。

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

## 初始化、所有权与失败

先初始化 I2C，再初始化 PMU；GX8002 先关闭核心/IO，设定 IO 3.3V、核心 1.0V，再按 IO → 10ms → 核心上电，关闭反序。音频先 init 再 open，再 read/write；音量为 0–100%，close 在电源休眠前完成。

共享总线只能由 BSP 初始化一次；生命周期调用由一个应用任务串行拥有。回调不能执行网络/音频/磁盘，按需复制到应用队列。音频读写可能阻塞，初始化失败返回错误，不能把缺失传感器当作成功。根工程不启动 Wi-Fi、蓝牙、云服务或产品状态机。

## 验证与真机验收

`./tools/build.sh` 构建默认固件及 Demo。编译与主机测试不能证明真机通过。真机需核对实体板号/芯片、Flash/PSRAM、引脚、总线地址、供电、音频非零录音与可听回放、按键/触摸/NFC（适用时）、LED 与 RTC/唤醒。电机、触觉、关机、休眠、4G 与唤醒固件升级只能显式测试，不能默认触发。

交付分别记录 `Build`、`Host tests`、`Device tests`、`Unverified`。首次刷入开发分区可能覆盖量产身份或数据，写入前明确设备、镜像与数据影响并取得授权。
