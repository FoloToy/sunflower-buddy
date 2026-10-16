---
name: sunflower-debug
description: sunflower-buddy 的 debug 工作流。
---

[English](SKILL.md)

# 硬件调试

读取 docs/BSP.zh_CN.md、受影响头文件与实现。核对实体板型、供电、芯片、Flash/PSRAM。将 I2C 地址、音频方向与 bsp_pins.h 比对。使用独立 Demo 隔离单个外设，记录脱敏日志、保留错误。回调不阻塞，禁止重复总线与生命周期所有者。先复现再改硬件常量，不照搬其他板卡数值。
