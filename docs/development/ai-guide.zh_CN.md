<p align="right"><a href="ai-guide.md">English</a> · <strong>简体中文</strong></p>

# AI 开发指南

先阅读 `AGENTS.zh_CN.md`，随后只读取本次功能对应的 BSP 公共头文件和实现。

```text
需求
  └─ main/main.cpp 事件路由与初始化
      ├─ main/demo_record_playback.cpp 音频/反馈示例
      ├─ main/demo_blufi.*         Wi-Fi/BLE 配网行为
      └─ components/bsp/include/   稳定板级 API
          └─ components/bsp/src/   硬件实现
              └─ bsp_pins.h        硬件事实唯一来源
```

可复用硬件能力放进 BSP，并通过精简的 `main/demo_*` 模块展示；事件路由保留在
`main.cpp`，Wi-Fi/BLUFI 状态保留在 `demo_blufi.*`。耗时操作必须放在应用任务中，
不能放在按键、蓝牙、Wi-Fi、定时器或中断回调中。复用现有 I2C 总线并保持明确的
供电顺序，不得从通用 ESP32-S3 开发板猜测本设备引脚或电气行为。

## 修改前

1. 运行 `git status --short --branch`，保留已有工作。
2. 阅读[硬件指南](../hardware-design/SUNFLOWER_BUDDY_V14_HARDWARE_GUIDE.zh_CN.md)对应章节。
3. 沿能力追踪公开头文件、实现、Demo 调用点、组件 manifest、生成配置默认值和仓库检查。
4. 任何会改变硬件行为的假设都应说明；修改引脚、电压、分区边界、持久格式或执行
   破坏性动作前必须确认。

## 实现边界

- 行为若持有外设、引脚、总线、供电时序或可复用板级策略，应新增 BSP 接口；仅属于
  用户流程的行为放在 Demo。
- ISR 只通知任务；组件回调只发送轻量事件。阻塞音频、延时、文件系统 I/O 和电源切换
  放在应用任务或 BSP 工作任务中。
- BLUFI 回调只校验并排队输入；Wi-Fi 配置、扫描、重连和清除凭据在配网任务执行。
  禁止记录密码、内置凭据，或把示例 DH 流程描述为产品级身份认证。
- 危险能力应保持可调用但不自动触发：没有明确示例需求时，Demo 不执行 PMU 关机、
  深睡、关闭充电、RTC 闹钟、模组会话或 GX8002 固件升级。
- 移动固件或调整存储规则后，保留 GX8002 文件并验证校验和。
- 除非明确改变项目范围且硬件得到确认，否则不增加未确认外设或产品/云功能。

## Review 清单

- 初始化和失败路径在文档承诺范围内可重复调用。
- 接触硬件前拒绝越界值和空指针。
- 共享 I²C、UART0 路由、LEDC 通道、ADC2、任务栈、PSRAM 和 LittleFS 所有权无冲突；
  Wi-Fi/BLE 生命周期与回调队列互不阻塞，也不会保留过期 BLUFI 序号/安全状态。
- 中英文文档描述相同最终行为。
- 编译、主机测试、真机测试和未验证硬件项分别报告。

交付前运行 `./tools/validate.sh --static`，并在 ESP-IDF 5.5.3 环境中运行
`./tools/validate.sh --firmware`。编译通过不等于设备验证，必须列出剩余真机检查。
