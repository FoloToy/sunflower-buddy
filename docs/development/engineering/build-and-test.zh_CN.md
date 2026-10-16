[English](build-and-test.md)

# Build and test

使用 ESP-IDF 5.5.3 / ESP32-S3。根工程只做最小初始化；完整诊断位于独立 Demo。两个工程有独立分区/构建目录，不能混用镜像。

```sh
. /path/to/esp-idf-v5.5.3/export.sh
./tools/validate.sh --static
./tools/validate.sh --firmware  # baseline + demo
# Individual builds:
idf.py build
idf.py -C examples/hardware-demo build
```

构建目录保留在各自工程中。默认输出 `build/sunflower-buddy.bin`，Demo 输出 `examples/hardware-demo/build/sunflower-hardware-demo.bin` 与 `storage.bin`。只在 Demo 中有 0x420000 的 LittleFS 分区；根工程无 storage。应用偏移为 0x10000，不是 0x0，实际写入以各自生成的 flash_args 为准。

依赖变更后同时审查两个 dependencies.lock，不修改 managed_components。编译、主机、真机与未验证项分别报告。实体板号、供电、音频、电机、RTC、唤醒和 BLE 仍需真机观察。
