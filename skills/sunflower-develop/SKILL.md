---
name: sunflower-develop
description: Develop an application or extend a board API for sunflower-buddy.
---

[简体中文](SKILL.zh_CN.md)

# Develop an application or extend a board API

Read AGENTS.md and docs/BSP.md. Check status and affected headers. Establish physical board/profile and authoritative pins. Put reusable peripheral operations in the BSP, application state downstream, diagnostics in examples. Document ownership, blocking, failure and initialization order. Never add cloud credentials, OTA or product state to the BSP. Run boundary check and both baseline/demo builds.
