#!/usr/bin/env python3
"""Validate the minimal Sunflower Buddy V1.4 BSP-reference repository contract."""

import hashlib
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def require(path: str, snippets: tuple[str, ...] = ()) -> None:
    target = ROOT / path
    if not target.is_file():
        raise AssertionError(f"missing required file: {path}")
    if not snippets:
        return
    text = target.read_text(encoding="utf-8")
    for snippet in snippets:
        if snippet not in text:
            raise AssertionError(f"{path}: missing {snippet!r}")


def require_sha256(path: str, expected: str) -> None:
    target = ROOT / path
    if not target.is_file():
        raise AssertionError(f"missing required file: {path}")
    actual = hashlib.sha256(target.read_bytes()).hexdigest()
    if actual != expected:
        raise AssertionError(f"{path}: expected SHA-256 {expected}, got {actual}")


def check_documentation() -> None:
    markdown = [*ROOT.glob("*.md"), *ROOT.glob("components/**/*.md"), *ROOT.glob("docs/**/*.md")]
    for target in markdown:
        if target.name.endswith(".zh_CN.md"):
            peer = target.with_name(target.name.removesuffix(".zh_CN.md") + ".md")
            expected_link = peer.name
            expected_label = "English"
        else:
            peer = target.with_name(target.stem + ".zh_CN.md")
            expected_link = peer.name
            expected_label = "简体中文"
        if not peer.is_file():
            raise AssertionError(f"missing bilingual peer for {target.relative_to(ROOT)}")
        text = target.read_text(encoding="utf-8")
        if expected_link not in text or expected_label not in text:
            raise AssertionError(f"missing language switch in {target.relative_to(ROOT)}")

        for link in re.findall(r"\[[^]]*\]\(([^)]+)\)", text):
            destination = link.split("#", 1)[0]
            if not destination or "://" in destination or destination.startswith("mailto:"):
                continue
            linked = (target.parent / destination).resolve()
            if not linked.exists():
                raise AssertionError(
                    f"broken link in {target.relative_to(ROOT)}: {destination}"
                )


def main() -> int:
    check_documentation()
    require("CMakeLists.txt", ('set(IDF_TARGET "esp32s3"', "project(sunflower-buddy)"))
    require("sdkconfig.defaults", ("CONFIG_PARTITION_TABLE_CUSTOM=y", "CONFIG_SENSOR_PMIC_SUPPORT=y"))
    require("components/bsp/include/bsp_pins.h", ("BSP_I2C_SDA", "BSP_GX8002_IRQ"))
    require("components/bsp/CMakeLists.txt",
            ("bsp_audio.cpp", "bsp_rtc.cpp", "bsp_modem.cpp",
             "bsp_sleep.cpp", "folotoy__folotoy-firmware-driver"))
    require("components/bsp/include/bsp_power.hpp",
            ("bsp_power_get_status", "bsp_power_set_charging_enabled", "bsp_power_shutdown"))
    require("components/bsp/include/bsp_motor.hpp", ("bsp_motor_enable_overcurrent_monitor",))
    require("main/main.cpp", ("demo_record_and_playback", "bsp_wakeup_set_callback", "xQueueSend"))
    require("main/demo_record_playback.cpp",
            ("bsp_audio_read", "bsp_button_is_pressed", "bsp_audio_write"))
    require("main/demo_blufi.c",
            ("esp_blufi_profile_init", "esp_wifi_set_config",
             "esp_timer_start_once", "COMMAND_RESET_PROVISIONING",
             "esp_ble_gatt_set_local_mtu"))
    require("main/main.cpp",
            ("BSP_BUTTON_LONG_PRESSED", "demo_blufi_reset_provisioning"))
    require("main/CMakeLists.txt", ("littlefs_create_partition_image(storage data",))
    require_sha256("main/data/gx_bl/grus-i2c.boot",
                   "8489b1c2e3428deee1ae8627116e3884652cad95928ba153c0e128371657ef77")
    require_sha256("main/data/gx_bl/mcu_nor.bin",
                   "393a1641eda927f3e45610458232d1d9e5cce27ed83a1f44de8270ff8a50c1f1")

    forbidden = ("application.cpp", "mcp_server.cpp", "mqtt_protocol.cpp")
    for name in forbidden:
        if list(ROOT.rglob(name)):
            raise AssertionError(f"production source must not be present: {name}")

    print("repository contract: PASS")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as error:
        print(f"repository contract: FAIL: {error}")
        raise SystemExit(1)
