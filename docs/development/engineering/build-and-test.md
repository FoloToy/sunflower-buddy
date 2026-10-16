[简体中文](build-and-test.zh_CN.md)

# Build and test

Use ESP-IDF 5.5.3 / ESP32-S3. Root is minimal bring-up; full diagnostics live in a separate demo. Each project owns its partition table and build directory: never mix images.

```sh
. /path/to/esp-idf-v5.5.3/export.sh
./tools/validate.sh --static
./tools/validate.sh --firmware  # baseline + demo
# Individual builds:
idf.py build
idf.py -C examples/hardware-demo build
```

Build outputs persist in each project. Baseline image: `build/sunflower-buddy.bin`. Demo: `examples/hardware-demo/build/sunflower-hardware-demo.bin` and `storage.bin`. Only the demo has LittleFS at 0x420000; root has no storage. Application offset is 0x10000, not 0x0; generated flash_args are authoritative for each project.

Review both dependencies.lock files after dependency changes; never edit managed_components. Report build, host tests, device tests and remaining checks separately. Board revision, power, audio, motor, RTC, wake and BLE still need device observations.
