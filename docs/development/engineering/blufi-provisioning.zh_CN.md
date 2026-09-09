<p align="right"><a href="blufi-provisioning.md">English</a> · <strong>简体中文</strong></p>

# BLUFI Wi-Fi 配网

本文记录 `main/demo_blufi.c` 的实际行为。BLUFI 属于 Demo 应用逻辑，不是可复用的
BSP 硬件 API。

## 范围与安全边界

- ESP32-S3 以 2.4 GHz Wi-Fi Station 工作；Demo 不包含 SoftAP、云协议、账号服务
  或 OTA。
- Station 凭据通过 BLUFI 下发，并由 ESP-IDF Wi-Fi 驱动保存到 NVS。固件不内置
  凭据，日志不会输出密码内容。
- 传输采用 ESP-IDF 5.5.3 示例的 DH 协商、MD5 派生 AES-CFB 密钥和 CRC。BLE SMP
  与 bonding 已关闭，本参考工程也没有启用 NVS 加密或建立产品级设备身份认证。
- 本地 GATT MTU 限制为 255 字节，因为 BLUFI 使用单字节记录载荷长度，可避免不兼容
  客户端产生回绕的帧长度。

该流程用于硬件参考和互操作验证，不等同于完整的量产配网安全设计。

## 生命周期

| 事件 | Wi-Fi 行为 | BLUFI 行为 |
| --- | --- | --- |
| 启动时存在已保存 SSID | 自动连接。 | 配网保持空闲，不开始广播。 |
| 启动时没有已保存 SSID | 保持 Station 运行以支持扫描。 | 以 `SunflowerBuddy-XXXX` 广播。 |
| 客户端连接 | 不改变 Wi-Fi 状态。 | 停止广播并创建新的安全上下文。 |
| 收到 SSID、密码和连接请求 | 持久化 Station 配置并连接。 | 向已连接客户端报告连接中、失败原因或成功。 |
| 取得 DHCP 地址 | 配网标记为完成，并重置重连退避。 | 没有客户端时停止广播；已有 GATT 连接目前会继续保留。 |
| 成功后客户端主动断开 | Wi-Fi 保持连接。 | 因配网已完成，不重新广播。 |
| 后续 Wi-Fi 断线 | 按 1、2、4、8、16 秒以及最长 30 秒持续重连。 | 不自动重新开放配网。 |
| 长按 Wi-Fi 键一秒 | 断网并只清空 Station 配置，保持或重启 Wi-Fi 以便扫描。 | 断开现有客户端以重置 BLUFI 报文序号与安全状态，然后恢复广播。 |

当前没有配网成功后的 BLE 超时，也没有关闭或反初始化 Bluedroid/BLE Controller。
如果手机一直保持 GATT 连接，该连接会持续到手机主动断开或长按 Wi-Fi 键。因此，
“不广播”“没有活动 GATT 连接”和“BLE Controller 已断电”是三个不同状态。

## 手机 App 流程

1. 扫描并连接 `SunflowerBuddy-XXXX`。
2. 完成 BLUFI 协商。
3. 让设备扫描并选择 2.4 GHz 热点。
4. 依次发送 SSID、密码和连接请求。
5. 等待设备报告 Wi-Fi 成功；失败时查看串口断线原因。

长按 Wi-Fi 键后，即使 App 界面仍显示旧设备，也必须重新连接。固件会主动关闭旧的
GATT 连接，让下一轮从报文序号 0 开始并重新协商密钥。

## 预期串口日志

首次配网成功通常包含：

```text
BLUFI client connected
BLUFI received station SSID (... bytes)
BLUFI received station password (... bytes)
BLUFI credentials saved; connecting to Wi-Fi
Wi-Fi connection attempt started
Wi-Fi connected, IP=...
```

日志只打印凭据长度，不打印密码内容。常见 Wi-Fi 断线原因：

| 原因 | 含义 | 首要检查 |
| --- | --- | --- |
| `201` | 找不到热点 | 确认开启 2.4 GHz、SSID 可见、信道和信号。 |
| `202` | 认证失败 | 重新输入密码并确认热点安全模式。 |
| `203` | 关联失败 | 检查热点容量、过滤规则和兼容性。 |
| `204` 或 `15` | 握手超时 | 检查密码、安全模式兼容性和射频质量。 |
| `210` | 没有兼容安全模式的热点 | 使用 ESP32-S3 Station 支持的安全模式。 |
| `212` | 热点低于 RSSI 阈值 | 靠近热点或改善 2.4 GHz 信号。 |

## 排障

| 日志或现象 | 解释与处理 |
| --- | --- |
| `Password length matches WPA2 standards...` | ESP-IDF 收到符合 WPA2 长度的密码后自动提高认证阈值，不代表密码失败。 |
| `BTM_BleWriteAdvData, Partial data write into ADV` | 传统广播载荷被截断；只要设备仍可发现和连接，就不是致命错误。 |
| `BT_HCI ... rsn 0x13` | 手机端正常主动终止 BLE 连接。 |
| `Invalid data length: 268, expected: 12` | 通常是旧固件与客户端 MTU 不兼容；确认固件包含 255 字节本地 MTU 限制后重新连接。 |
| `seq 0 is not expect N` | App 在旧 GATT 会话内重启了 BLUFI 序号；长按重置，等待断开，再重新连接后配网。 |
| 扫描出现 `ESP_ERR_WIFI_NOT_STARTED` | Wi-Fi Station 未运行。当前重置逻辑只清空 Station 配置并会重启 Wi-Fi；确认已烧录当前固件。 |
| 密码正确但没有发起连接 | 确认日志到达 `credentials saved` 和 `connection attempt started`；旧固件可能在 Station 已空闲时永久等待断线事件。 |
| 成功后 App 仍显示设备 | 当前固件保留已连接的 GATT 客户端且没有自动关闭定时器；在 App 中断开或长按 Wi-Fi 重置。 |

## 真机验收

记录固件 commit 和手机 App 版本，并验证：

- 清空 Station 凭据后完成配网并取得 IP，日志中不出现密码内容；
- 重启后使用已保存配置自动连接；
- 中断热点后确认指数退避重连，DHCP 成功后退避复位；
- 分别在未连接和已有 BLUFI 客户端时长按 Wi-Fi，确认凭据清除、旧 GATT 连接关闭、
  扫描正常，并可用新会话再次配网；
- 至少连续执行三次重置和配网，不出现帧长度、报文序号或
  `WIFI_NOT_STARTED` 错误；
- 单独记录手机 App 是否在成功后关闭 GATT，因为固件当前不强制超时。
