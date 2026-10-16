[English](VALIDATION.md)

# Validation record — 2026-10-16

本机编译与源代码检查，未连接实体硬件。

- Build: PASS — ESP-IDF 5.5.3; baseline and independent `examples/hardware-demo`.
- Host tests: PASS — actual button adapter compiled with fault-injected drivers (allocation/callback failures, cleanup/retry, repeated init, invalid indices).
- Architecture checks: PASS — minimal default entry, BSP/application boundary, public dependency URLs and literal-credential guard.
- Device tests: NOT RUN.
- Unverified: 实体板号、引脚/供电、Flash/PSRAM、I2C、电池、按键、旋钮、音频声道/增益/可听回放、板载外设，以及适用的电机/触觉/NFC/触摸/RTC/唤醒/4G。具体能力与已知边界见 BSP 指南。

复现：`./tools/build.sh`；仅主机边界检查：`python3 tools/check_boundaries.py`。构建可通过公开依赖独立复现。
