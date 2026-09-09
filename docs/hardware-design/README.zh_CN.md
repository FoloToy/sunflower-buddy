<p align="right"><a href="README.md">English</a> · <strong>简体中文</strong></p>

# 硬件设计

本目录维护固件可见的 Sunflower Buddy V1.4 硬件事实、资源约束、初始化顺序、排障和真机验收。

## 文档地图

| 文档 | 权威范围 |
| --- | --- |
| [规格](specifications.zh_CN.md) | 已确认板级能力和软件支持边界。 |
| [Sunflower Buddy V1.4 硬件指南](SUNFLOWER_BUDDY_V14_HARDWARE_GUIDE.zh_CN.md) | 引脚用途、总线、时序、运行约束、排障和验收。 |
| [`bsp_pins.h`](../../components/bsp/include/bsp_pins.h) | 固件引脚、地址、时钟与板级配置常量的唯一来源。 |
| [BSP README](../../components/bsp/README.zh_CN.md) | 公开 API、资源所有权、初始化和调用上下文。 |
| [BLUFI 配网指南](../development/engineering/blufi-provisioning.zh_CN.md) | 应用层 Wi-Fi/BLE 生命周期、安全边界、排障和射频验收。 |

不得根据通用 ESP32-S3 能力或其他 FoloToy 板卡推断本板接线。编译只能证明软件兼容，
物理行为必须在 Sunflower Buddy V1.4 真机验收。
