<p align="right"><a href="CHANGELOG.md">English</a> · <strong>简体中文</strong></p>

# 变更记录

## 未发布

### 新增

- 新建独立的 Sunflower Buddy 工程。
- 增加 Sunflower Buddy V1.4 的 I2C、按键、音频、灯、PMU、电机和 GX8002 最小 BSP。
- 增加一个按住录音、松开回放的 Demo，并包含板级状态及灯光/电机反馈。
- 增加采用示例加密方案的 BLUFI Wi-Fi 配网、已保存配置自动连接、指数退避重连、长按清除凭据，
  并限制 GATT MTU 以兼容客户端的 BLUFI 单字节长度字段。
- 在 LittleFS 镜像中保留 Sunflower 2 的 GX8002 唤醒词固件。
- 移除产品固件、云协议、发布脚本、内置凭据和媒体资源。
- 固定 ESP32-S3 目标，并增加中英文 AI、硬件和构建说明。

### 变更

- 将 FoloToy 固件驱动依赖切换到公开的 `folotoy-firmware-driver-open-source`
  仓库，并更新依赖锁文件。

### 修复

- Wi-Fi Station 已空闲时立即应用首次 BLUFI 凭据，不再等待可能永远不出现的断线事件。
- 清除凭据后保持 Station Wi-Fi 运行，并重启已有 BLUFI 连接，使报文序号和安全状态
  从干净会话开始。

### 文档

- 完善双语文档，增加能力契约、BSP API 地图、环境搭建、代码与贡献规范、排障、
  Flash 布局和真机验收清单。
- 新增独立双语 BLUFI 生命周期/安全/排障指南，并让项目、BSP、构建、硬件、贡献和
  Agent 说明与实际固件保持一致。
