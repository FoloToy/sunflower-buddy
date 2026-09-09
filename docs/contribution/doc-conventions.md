<p align="right"><strong>English</strong> · <a href="doc-conventions.zh_CN.md">简体中文</a></p>

# Documentation Conventions

- Every maintained Markdown document uses English at `name.md` and Simplified
  Chinese at `name.zh_CN.md`; both pages link to each other at the top.
- Keep headings, hardware facts, commands, safety notes, and links aligned in the
  same change.
- `bsp_pins.h` is authoritative for firmware-visible pins and addresses. Link to
  it instead of creating a second table in application documentation.
- Root READMEs introduce the project. `docs/README*` indexes documentation;
  `development/` owns workflows; `hardware-design/` owns hardware facts and
  acceptance; `contribution/` owns collaboration rules.
- Explain constraints, failure modes, side effects, and validation. Do not merely
  restate source code.
- Never include credentials, device identities, private endpoints, personal data,
  or unredacted serial numbers.
- Before publishing serial logs, redact SSIDs, BSSIDs/MAC addresses, IP
  addresses, modem identifiers, tokens, and
  device-unique suffixes. Preserve error codes, lengths, timing, and state
  transitions needed to reproduce the issue.
- Do not claim unsupported capabilities. This repository contains no cloud
  protocol, OTA pipeline, production application, or unconfirmed peripheral.
