<p align="right"><strong>English</strong> · <a href="README.zh_CN.md">简体中文</a></p>

# Development

This directory contains the engineering rules for the Sunflower Buddy V1.4 hardware
reference. They describe the repository as it exists; they are not production
firmware or cloud-service instructions.

## Document map

- [AI development guide](ai-guide.md): context routing, BSP boundaries, task and
  callback rules, and delivery expectations.
- [Environment setup](engineering/environment-setup.md): ESP-IDF 5.5.3 setup and
  checkout initialization.
- [Build and test](engineering/build-and-test.md): local validation, flashing,
  monitoring, and result reporting.
- [BLUFI provisioning](engineering/blufi-provisioning.md): implemented lifecycle,
  security boundary, mobile flow, diagnostics, and repeated-provisioning tests.
- [Coding conventions](engineering/coding-conventions.md): C/C++ style,
  ownership, concurrency, hardware constants, and test expectations.

Hardware facts belong in [the hardware-design section](../hardware-design/README.md).
Contribution and documentation rules belong in
[the contribution section](../contribution/README.md).
