[English](README.md)

# 小葵 2.0

面向 AI Agent 的公开硬件 BSP，按 [AI Passport](https://github.com/FoloToy/ai-passport) 的硬件 API、引脚事实源、上下文路由与验证分层设计组织。

默认 `main/main.cpp` 只初始化总线、电源和 LED，不启动应用服务。完整硬件演示独立位于 `examples/hardware-demo`，不会进入默认固件。

## 开始开发

先读 [AGENTS.md](AGENTS.md)，再按任务读取 [硬件与 API 指南](docs/BSP.zh_CN.md)。改硬件前确认实体板卡版本，不根据产品名猜引脚。

```text
components/bsp/include/   public hardware API; bsp_pins.h is authoritative
components/bsp/src/       reusable board adaptation
main/                     minimal baseline
examples/hardware-demo/   independent diagnostic application
docs/                     hardware, ownership and validation context
tools/build.sh            builds both firmware projects
```

## 构建

```sh
. /path/to/esp-idf-v5.5.3/export.sh
./tools/build.sh
# Or build one project:
idf.py build
idf.py -C examples/hardware-demo build
```

固定使用 ESP-IDF 5.5.3，目标为 esp32s3。仅使用公开依赖，驱动固定到提交。默认分区只有 NVS、PHY 和一个 factory 应用，Demo 可有独立分区。不要把开发分区直接当成量产升级布局。

## 范围与验证

BSP 不包含 MQTT、TTS、OTA、云端凭据或产品状态机。初始化顺序、回调上下文、阻塞行为、板卡差异及真机验收见硬件指南。编译、主机测试、真机测试分别报告。MIT 许可证仅适用于本项目源码，第三方依赖保留原许可证。
