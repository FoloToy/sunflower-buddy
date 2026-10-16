---
name: sunflower-develop
description: sunflower-buddy 的 develop 工作流。
---

[English](SKILL.md)

# 开发

先读 AGENTS.zh_CN.md、docs/BSP.zh_CN.md 与受影响的 API 头文件。确认实体板卡和唯一引脚表。外设复用能力放 BSP，产品状态放下游应用，诊断放 examples。写清初始化顺序、资源所有权、阻塞和失败行为；不引入云凭据、OTA 或角色业务。完成后运行边界检查及默认/Demo 构建。
