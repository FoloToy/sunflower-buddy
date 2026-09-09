<p align="right"><a href="AGENTS.md">English</a> · <strong>简体中文</strong></p>

# 仓库开发约定

- 本仓库是 Sunflower Buddy V1.4 硬件参考工程，不是完整产品固件。
- 仅支持 ESP-IDF 5.5.3 和 ESP32-S3。
- 开始工作先执行 `git status --short --branch`，保留无关修改。
- 硬件常量只能定义在 `components/bsp/include/bsp_pins.h`。
- 可复用硬件接口与实现放入 `components/bsp`。
- 最小可运行示例和应用行为放入 `main/demo_*`。
- Wi-Fi/BLUFI 配网保留在 `main/demo_blufi.*`；禁止输出凭据内容，新一轮配网前必须
  重置已有 BLUFI 连接。
- 不添加云端协议、内置凭据、OTA、产品状态机或媒体资源包。
- 按键及中断回调不得阻塞。
- 所有设备复用 BSP 管理的 I2C0，禁止创建第二个总线所有者。
- 不得改变 GX8002 的电压、供电顺序及 10 ms 延时。
- 分别报告编译、主机测试、设备测试和未验证硬件项。

修改硬件相关代码前阅读 `docs/hardware-design/SUNFLOWER_BUDDY_V14_HARDWARE_GUIDE.zh_CN.md`；
修改 Wi-Fi/BLE 配网前阅读
`docs/development/engineering/blufi-provisioning.zh_CN.md`。
