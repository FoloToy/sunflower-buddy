<p align="right"><a href="SUNFLOWER_BUDDY_V14_HARDWARE_GUIDE.md">English</a> · <strong>简体中文</strong></p>

# Sunflower Buddy V1.4 硬件指南

事实源优先级：实测/产品规格、`components/bsp/include/bsp_pins.h`、BSP 适配实现、
本文档。

## 平台

| 项目 | 配置 |
| --- | --- |
| MCU | ESP32-S3，16 MB Flash，8 MB Octal PSRAM |
| 无线参考 | 2.4 GHz Wi-Fi Station 与 BLE 4.2 BLUFI |
| 音频 | ES8311，输入输出 24 kHz；ESP TX GPIO3、RX GPIO9；NS4150 使能 GPIO15 |
| 电源 | AXP2101，I2C 地址 `0x34`，中断 GPIO2 |
| 电机 | MX6115 PWM GPIO47/GPIO38，过流输入 GPIO14 |
| 灯光 | 可寻址 RGB 数据 GPIO6 |
| LTE 串口 | 物理 TX GPIO4、RX GPIO5；旧适配层有意交换 DTE RX/TX 配置 |
| 共用 I2C0 | SDA GPIO41、SCL GPIO42 |
| RTC | BM8563，地址 `0x51`，中断 GPIO1 |
| 唤醒芯片 | GX8002 中断 GPIO13 |

四个按键均为低有效：模式 GPIO21、语音 GPIO12、音量加 GPIO7、音量减 GPIO16。
深睡唤醒掩码还包含 BSP 中定义的 PMU、GX8002 和 RTC 输入。

## 不变量

- GX8002 VDDIO 必须设为 3.3 V 并先于 1.0 V Core 开启；关闭时顺序相反，并保留
  代码中注明的延时。
- PMU 休眠前必须停止 codec 输入输出，防止 ES8311 持续占用 I2C 总线。
- 音量以 0–100 百分比表示，并在 codec 创建后设置。
- LTE 已知初始波特率为 115200，运行目标波特率为 921600。
- Wi-Fi 配网只面向 2.4 GHz Station；不得把 5 GHz、SoftAP、BLE SMP、bonding 或
  加密 NVS 写成已实现能力。
- 本板没有显示屏，不得对外声明 LCD/LVGL 能力。

## 资源所有权与初始化

I2C0 只由 `bsp_i2c_init()` 初始化一次。AXP2101、BM8563、GX8002 和 ES8311 共用
该总线；正常 Demo 扫描通常应看到 `0x18`、`0x2F`、`0x34` 和 `0x51`。地址缺失
表示设备、供电或总线故障，不能通过再创建一条同端口总线解决。

PMU 必须先于 GX8002 初始化，因为 GX8002 电源轨由 PMU 控制。codec 在 I²C 后初始化，
PCM 读写前先打开格式。灯、电机和按键驱动分别持有 GPIO/RMT/LEDC/button 资源。
LTE UART 为显式启用，因为它会把 UART0 路由到 GPIO5/GPIO4，可能改变 console 行为。

## 外设行为

### AXP2101 电源

BSP 暴露电量百分比和电压、系统/VBUS 电压、电池与 VBUS 状态、充电状态、开关机来源、
充电开关、PMU 事件、休眠和关机。PMU GPIO2 ISR 只唤醒工作任务；I²C 寄存器读取和
应用回调均在 ISR 外执行。事件掩码包含低电量、电池/VBUS 插拔、开始/完成充电、温度
故障、过流、充电超时和电源键动作。

`bsp_power_shutdown()` 与 `bsp_power_enter_sleep()` 会改变硬件状态，因此没有绑定到
Demo 按键。`bsp_power_driver()` 为需要稳定 API 未封装 AXP2101 操作的高级板级实验提供
底层入口；使用它的代码会与 Managed Driver 版本耦合。

### 音频

I2S0 使用 BCLK GPIO10、WS GPIO46、MCLK GPIO11、ESP TX GPIO3、ESP RX GPIO9。
已验证 codec 配置为从模式且 `use_mclk=false`，ES8311 从 BCLK 获取内部时钟。Demo
打开 24 kHz、16-bit、单声道 PCM，输入增益 30 dB，初始输出音量 60%。最长 15 秒录音
在 PSRAM 占用 720,000 字节。红灯表示录音，绿灯表示回放。

### 按键与 Demo 流程

四个按键均为低有效。BSP 上报按下、释放、点击和长按事件，Demo 将回调排队到一个
应用任务：

- 按住对话键录音，松开后回放捕获的 PCM；
- 音量键以 10% 步进调整，限制在 0–100%；
- 单击 Wi-Fi/模式键输出状态、白灯亮 250 ms，并让电机单次动作；
- 长按 Wi-Fi/模式键 1 秒会排队清除已保存凭据并重新开启 BLUFI 配网；
- GX8002 事件让电机以 1023 占空比动作 500 ms。

按键回调只负责投递长按事件；Wi-Fi 断开、NVS 更新和 BLUFI 广播均由应用任务执行。

### BLUFI Wi-Fi 配网

Demo 启用 ESP32-S3 Station Wi-Fi 和仅 BLE 的 BLUFI。Station 配置由 ESP-IDF 保存
在 NVS 中；固件不内置 SSID 或密码，也不会把收到的密码写入日志。BLUFI 采用
ESP-IDF 5.5.3 示例的 DH 密钥协商、AES-CFB 加密和 CRC 校验。

启动时如存在已保存 SSID，Demo 会自动发起连接。后续每次断线均持续重连，退避时间
依次为 1、2、4、8、16 秒，最长 30 秒；成功取得 DHCP 地址后重置退避。没有保存凭据
时，BLUFI 自动广播，名称为 `SunflowerBuddy-XXXX`，其中 `XXXX` 是蓝牙 MAC 地址最后
两字节。长按 Wi-Fi/模式键 1 秒只清除 Wi-Fi 驱动的持久化配置并返回配网，不擦除
其他 NVS 数据。如已有 BLUFI 客户端连接，重置会断开该连接，使报文序号和安全协商
从干净状态重新开始；随后需要重新连接广播中的设备。

DHCP 成功后配网状态变为空闲；客户端断开后不会重新广播，但当前参考实现会保留已经
连接的 GATT 客户端，并保持 Bluedroid/BLE Controller 初始化，没有成功后的自动关闭
定时器。准确状态表、安全边界、日志和重复会话测试见
[BLUFI 配网指南](../development/engineering/blufi-provisioning.zh_CN.md)。

BLUFI 只负责本地 Station 配网；Demo 不包含 SoftAP、应用协议、远程服务、OTA 或云连接。

### 电机与过流输入

MX6115 使用 GPIO47/GPIO38、20 kHz、10-bit LEDC PWM，公开占空比为 0–1023。回放
开始时以 1023 持续正转，PCM 输出结束后排队停止。GPIO14 对应 ESP32-S3 ADC2 通道3。
监测任务每 20 ms 采样，以 0.9/0.1 指数移动平均滤波；默认原始阈值 2500，连续超限
100 ms 后停止电机，下降到阈值以下 200 个计数后复位。这些值只是基线，不能解释为
已经标定的毫安电流。

### GX8002

LittleFS `storage` 镜像包含 `gx_bl/grus-i2c.boot` 和 `gx_bl/mcu_nor.bin`，Demo
挂载到 `/littlefs/gx_bl`。初始化只查询现有固件并安装低有效 GPIO13 回调，不会刷写固件。
恢复升级必须显式调用：

```cpp
bsp_wakeup_upgrade("/littlefs/gx_bl/grus-i2c.boot",
                   "/littlefs/gx_bl/mcu_nor.bin");
```

升级期间不得断电，之后应在设备上验证固件校验和与版本。

### RTC 与深睡

BM8563 支持设置系统时间、日历闹钟、相对定时闹钟、清除闹钟和 RTC 唤醒判断。默认
EXT1 ANY_LOW 唤醒掩码包含四个按键、PMU GPIO2、GX8002 GPIO13 和 RTC GPIO1。
`bsp_sleep_enter()` 配置掩码、关闭 codec 输入输出、让 AXP2101 进入板级休眠，再启动
ESP 深睡。

### LTE UART

BSP 只提供 UART 初始化、修改波特率、flush 和字节收发，不包含 AT 解析、自动波特率
协商、PPP、联网或云协议。已知模组启动波特率为 115200，期望运行波特率为 921600。
高层实现必须先在当前速率获得 `OK`，再修改模组与主控两端，并处理模组已经处于 PPP
数据模式的情况。

## Flash 与内存布局

| 偏移 | 大小 | 用途 |
| --- | --- | --- |
| `0x9000` | 24 KB | NVS |
| `0xF000` | 4 KB | PHY 初始化数据 |
| `0x10000` | 4 MB | Factory 应用 |
| `0x410000` | 64 KB | Core dump |
| `0x420000` | 512 KB | GX8002 固件 LittleFS 存储 |

当前分区在 `0x4A0000` 结束，16 MB Flash 的其余空间在本参考布局中有意保持未分区。
没有修改并 review `partitions.csv` 前，不得自行声明用途。

Demo 依赖 Octal PSRAM。分配失败时本次录音失败，不会回退到大块内部 RAM 缓冲区。

## 排障

| 现象 | 检查项 |
| --- | --- |
| 录音 peak 为 0 或回放无声 | 确认扫描到 `0x18`、GPIO3 TX/GPIO9 RX 方向、codec open 返回值、麦克风供电和 I²S 时钟。 |
| 音量变化但仍很小 | 确认 0–100 数值直接传给 `bsp_audio_set_volume()` 且未二次缩放；检查 PA GPIO15 和扬声器。 |
| 电机实际动作但 API 报错 | 使用 BSP 包装；上游队列 API 的 `pdPASS` 整数值为 1，必须规范化。 |
| 电机意外停止 | 记录 GPIO14 raw/filtered 数值，在已知负载下标定 OCP 阈值。 |
| GX8002 地址 `0x2F` 消失 | 先检查 AXP2101，再确认 3.3 V I/O → 10 ms → 1.0 V Core 顺序。 |
| RTC 闹钟反复触发 | 在任务上下文清除 BM8563 标志，并确认 GPIO1 恢复非活动状态。 |
| 向模组发送 `AT` 却收到二进制数据 | 模组很可能处于 PPP 数据模式，应按模组保护时间执行退出数据模式流程。 |
| 深睡后立即唤醒 | 读取 EXT1 唤醒状态，找出进入休眠前已经拉低的来源。 |
| 找不到 `SunflowerBuddy-XXXX` | 确认 BLE/BLUFI 初始化成功且没有有效的已保存 SSID，或长按 Wi-Fi/模式键 1 秒重启配网。 |
| Wi-Fi 反复断线 | 从串口日志读取断线原因，检查热点凭据与 2.4 GHz 信号，并观察 1–30 秒重连退避。 |
| 重复配网出现 `seq 0 is not expect N` | 手机复用了旧 GATT 会话；长按 Wi-Fi，等待 BLE 被强制断开，重新连接后再开始配网。 |
| BLUFI 扫描出现 `ESP_ERR_WIFI_NOT_STARTED` | 确认已烧录当前固件；重置应只清空 Station 配置，不能恢复并停止整个 Wi-Fi 栈。 |
| 出现 `gpio_install_isr_service... already installed` | PMU 与 RTC 复用 ESP-IDF 全局 GPIO ISR 服务；若两个 BSP 初始化均成功且中断正常，该二次安装提示可视为信息，禁止再安装另一套服务。 |
| 启动出现 `Incorrect size of core dump image` | coredump 分区含旧数据或非 coredump 内容，不代表本次启动崩溃；保留用于排查，或明确只擦除该分区。 |

## 设备验收

记录板卡版本、固件 commit、电源来源、电池状态和测试条件。至少验证：

- 四个预期 I²C 地址均出现，重复访问稳定；
- 每个按键均产生按下/释放/点击/长按，且不阻塞任务；
- 语音录音 peak 非零，回放可听且无明显失真；
- 音量从相反边界按十次可达到 0% 和 100%；
- RGB 颜色和白灯时长符合请求；
- 电机启停、持续、单次、方向正常，OCP 在受控负载下能停止电机；
- PMU 状态以及插拔、充电、电源键事件与物理动作一致；
- RTC 时间、闹钟中断、清除闹钟和 RTC 深睡唤醒正常；
- GX8002 版本、启停、唤醒中断、电源循环和显式恢复升级正常；
- 模组在预期波特率进入命令模式，UART0 与 console 共存关系已确认；
- 每个默认 EXT1 来源均能唤醒，非活动来源不会造成唤醒循环；
- 全新设备广播 `SunflowerBuddy-XXXX`，可接收 BLUFI 凭据、取得 IP，并在客户端
  断开后停止广播；
- Wi-Fi 丢失后会自动退避重连，重启后从 NVS 自动连接，长按 Wi-Fi/模式键 1 秒会
  清除凭据并恢复 BLUFI 广播；
- 连续三次清除并重新配网都会关闭旧 GATT 会话，且不出现帧长度、报文序号或 Wi-Fi
  已停止错误。

PMU 关机、关闭充电、深睡、GX8002 升级、电机堵转/OCP 和模组波特率修改属于破坏性
或高风险测试，必须有意执行。编译通过不等于真机验证通过。
