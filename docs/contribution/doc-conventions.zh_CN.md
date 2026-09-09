<p align="right"><a href="doc-conventions.md">English</a> · <strong>简体中文</strong></p>

# 文档规范

- 所有维护中的 Markdown 文档默认 `name.md` 使用英文，简体中文使用
  `name.zh_CN.md`，并在顶部互相链接。
- 同一次修改中保持章节、硬件事实、命令、安全说明和链接一致。
- 固件可见的引脚和地址以 `bsp_pins.h` 为权威来源，应用文档应链接引用，不创建第二份
  相互竞争的常量表。
- 根 README 介绍项目，`docs/README*` 负责索引，`development/` 维护工作流，
  `hardware-design/` 维护硬件事实和验收，`contribution/` 维护协作规则。
- 文档应解释约束、失败方式、副作用和验证方法，不要只复述源码。
- 禁止写入凭据、设备身份、私有 endpoint、个人数据或未脱敏序列号。
- 对外发布串口日志前，脱敏 SSID、BSSID/MAC、IP 地址、模组标识、
  token 和设备唯一后缀；保留复现问题所需的错误码、长度、时间和状态转换。
- 不得声明未支持能力。本仓库不包含云协议、OTA 流程、产品应用或未确认外设。
