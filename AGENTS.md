[简体中文](AGENTS.zh_CN.md)

# Agent development entry

Start with `git status --short --branch` and preserve user changes. Route to `docs/BSP.md` and affected headers, rather than loading all documentation.

- Hardware authority: measurement/specification → `bsp_pins.h` → implementation → docs.
- Reusable hardware APIs belong in `components/bsp`; product applications belong downstream. Keep default main minimal; diagnostics belong in `examples/hardware-demo`.
- Do not copy private product applications, credentials, identity partitions or cloud logic. Pins and device addresses belong only in `bsp_pins.h`.
- Callbacks copy/queue data without blocking; never perform I2C, audio or network work in ISR context. Serialize lifecycle calls from one application owner.
- Reuse BSP-owned buses. Preserve board-specific PMU/GX8002 voltages, rail order and delays.
- Build baseline and standalone demo with `./tools/build.sh`. Keep English/Chinese docs aligned.
- Report Build, Host tests, Device tests and Unverified separately. Compilation is not device validation; obtain authorization for the exact device/image before flashing.

[Read the matching repository Agent skill](skills/README.md).
