---
name: sunflower-build
description: Compile the baseline and its standalone diagnostic demo for sunflower-buddy.
---

[简体中文](SKILL.zh_CN.md)

# Compile the baseline and its standalone diagnostic demo

Run python3 tools/check_boundaries.py. Activate ESP-IDF 5.5.3 and run ./tools/build.sh. It builds root and examples/hardware-demo separately. Inspect compiler exit codes, warnings and image/partition fit. Do not infer runtime behavior from compilation. Record Build, Host tests, Device tests and Unverified separately.
