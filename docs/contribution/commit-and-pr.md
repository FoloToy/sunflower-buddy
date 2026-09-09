<p align="right"><strong>English</strong> · <a href="commit-and-pr.zh_CN.md">简体中文</a></p>

# Commit and PR Rules

## Commits

- Use an English imperative Conventional Commit title such as
  `feat(bsp): add RTC alarm wrapper`.
- Keep one independently reviewable change per commit and describe the final
  diff, not the debugging history.
- Before committing, inspect the complete diff, run applicable validation, and
  exclude credentials, local configuration, build outputs, and unrelated files.
- Update the changelog for user-visible behavior or compatibility changes.
- Commit authorization does not imply permission to push, publish, merge, tag,
  or create a release.

## Pull requests

- State the Sunflower Buddy hardware revision and any pin, power, codec, ADC, UART, Flash,
  or timing impact.
- Report Build, Host tests, Device tests, and Unverified items separately.
- Include serial logs or measurements for hardware validation. Include the exact
  test setup when changing motor current thresholds, audio gain, modem baud, or
  low-power behavior.
- For Wi-Fi/BLUFI changes, record the mobile-app version, AP band/security,
  firmware commit, cleared/saved station configuration, repeated reset count,
  and observed disconnect reasons. Redact credentials and identifiers.
- Never claim device validation from a successful compiler or host test.
