<p align="right"><a href="README.md">English</a> · <strong>简体中文</strong></p>

# 组件

当前基线只有一个仓库自有组件 `bsp`。可复用硬件支持放入 `components/bsp`，产品逻辑
和 Demo 行为保留在 `main`。ESP Component Manager 依赖由
`components/bsp/idf_component.yml` 与 `main/idf_component.yml` 声明，解析结果锁定在
`dependencies.lock`，并生成到已忽略的 `managed_components/`；禁止直接修改生成目录。

[BSP 模块指南](bsp/README.zh_CN.md)说明公开 API、初始化顺序、调用上下文和硬件约束。
