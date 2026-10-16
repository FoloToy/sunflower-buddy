---
name: sunflower-build
description: sunflower-buddy 的 build 工作流。
---

[English](SKILL.md)

# 构建

先运行 `python3 tools/check_boundaries.py`，再执行 `./tools/build.sh`。先激活 ESP-IDF 5.5.3；默认工程和 examples/hardware-demo 必须分别构建。检查退出码、警告及镜像/分区适配。编译、主机测试、真机测试与剩余验证分别报告，不能用编译推断硬件行为。
