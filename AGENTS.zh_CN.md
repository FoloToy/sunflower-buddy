[English](AGENTS.md)

# Agent 开发入口

先运行 `git status --short --branch`，保留用户修改。按任务读取 `docs/BSP.zh_CN.md` 和相关头文件，不需要一次读取全部文档。

- 以测量/规格书 → `bsp_pins.h` → BSP 实现 → 文档为硬件事实优先级。
- 硬件 API 放 `components/bsp`；产品应用在下游工程，默认 main 保持最小；演示放 `examples/hardware-demo`。
- 不复制私有产品源码、凭据、身份分区或云逻辑。引脚、地址只在 `bsp_pins.h` 定义。
- 回调仅复制/入队，不阻塞，不在中断中执行 I2C、音频或网络。生命周期由一个应用任务串行调用。
- 复用 BSP 总线，禁止重复所有者。PMU 与 GX8002 的电压、顺序和延时不得从其他板型套用。
- 修改后构建默认工程和 Demo：`./tools/build.sh`。中英文文档同步。
- 交付分别报告 Build、Host tests、Device tests、Unverified；无真机结果不得声称硬件通过。烧录前需针对具体设备/镜像取得授权。

[按任务读取仓库内 Agent skills](skills/README.zh_CN.md)。
