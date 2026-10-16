---
name: sunflower-device-test
description: sunflower-buddy 的 device-test 工作流。
---

[English](SKILL.md)

# 真机测试

先读 docs/BSP.zh_CN.md，只读发现串口，不打开/复位无关设备。写入前确认具体板号、镜像、分区与数据影响并取得用户授权。不得默认擦除身份分区。逐项核对供电、I2C、按键、音频及已安装外设，分别记录串口证据和人工观察。关机、电机/触觉、休眠、蜂窝网络、唤醒芯片升级均需有意测试；未观察到的项目不能标为通过。
