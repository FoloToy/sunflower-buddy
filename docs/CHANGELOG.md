<p align="right"><strong>English</strong> · <a href="CHANGELOG.zh_CN.md">简体中文</a></p>

# Changelog

## Unreleased

### Added

- Created the standalone Sunflower Buddy project.
- Added a minimal Sunflower Buddy V1.4 BSP for I2C, buttons, audio, LED, PMU, motor, and GX8002.
- Added a single hold-to-record, release-to-play demo with board status and
  light/motor feedback.
- Added example-encrypted BLUFI Wi-Fi provisioning, saved-configuration auto-connect,
  exponential reconnect, long-press credential reset, and a protocol-safe GATT
  MTU cap for client compatibility.
- Preserved the Sunflower 2 GX8002 wake-word firmware in the LittleFS image.
- Removed product firmware, cloud protocols, release packaging, embedded credentials, and media assets.
- Fixed the project to ESP32-S3 and added bilingual AI/hardware/build guidance.

### Changed

- Switched the FoloToy firmware driver dependency to the public
  `folotoy-firmware-driver-open-source` repository and refreshed the lock file.

### Fixed

- Applied first-time BLUFI credentials immediately when the Wi-Fi station is
  idle instead of waiting for a disconnect event that may never arrive.
- Kept station Wi-Fi running after credential reset and restarted an active
  BLUFI connection so packet sequence and security state begin cleanly.

### Documentation

- Expanded bilingual documentation with a capability contract, BSP API map,
  environment setup, coding and contribution rules, troubleshooting, Flash
  layout, and a device-acceptance checklist.
- Added a dedicated bilingual BLUFI lifecycle/security/diagnostics guide and
  aligned project, BSP, build, hardware, contribution, and agent guidance with
  the implemented firmware.
