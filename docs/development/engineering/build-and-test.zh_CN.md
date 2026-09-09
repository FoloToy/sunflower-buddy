<p align="right"><a href="build-and-test.md">English</a> · <strong>简体中文</strong></p>

# 构建与测试

使用 ESP-IDF 5.5.3，目标为 ESP32-S3。

## 增量构建

```bash
source /path/to/esp-idf-v5.5.3/export.sh
idf.py build
```

新 checkout 或有意改变生成 target 状态时运行一次 `idf.py set-target esp32s3`。顶层
CMake 同样强制 ESP32-S3；每次构建都运行 `set-target` 会无意义地重新生成
`sdkconfig`。

预期产物包括：

- `build/sunflower-buddy.bin`：烧录到 `0x10000` 的应用镜像；
- `build/storage.bin`：烧录到 `0x420000` 的 GX8002 固件 LittleFS 镜像；
- `idf.py flash` 使用的 bootloader 和分区表镜像。

不能把应用单镜像烧录到 `0x0`。

## 验证

统一验证入口：

```bash
./tools/validate.sh --static
./tools/validate.sh --firmware
./tools/validate.sh
```

静态验证检查单板 BSP/Demo 仓库契约和主机测试。固件验证使用隔离的临时
构建目录，不修改开发者根目录下的 `sdkconfig` 和 `build/`；临时固件会在验证后删除，
需要持久烧录产物时运行 `idf.py build`。
首次固件验证可能下载锁定的 Managed Component，因此需要 registry/Git 访问。

仓库提交 `dependencies.lock`。修改 `idf_component.yml` 后，使用 ESP-IDF 5.5.3
构建、检查依赖变化，并一起提交 manifest 与锁文件。禁止修改 `managed_components/`。

## 烧录与监视

```bash
idf.py -p <port> flash monitor
```

当前分区对应偏移为 bootloader `0x0`、分区表 `0x8000`、应用 `0x10000`、LittleFS
存储 `0x420000`。优先使用 `idf.py flash`，以生成的 flash 参数为准。

## 阅读构建与启动输出

- 查找第一条 `FAILED`、`fatal error` 或 CMake 诊断；最后的
  `ninja: build stopped` 只说明前面已有命令失败。
- bootloader 成功不代表应用已经链接；还要确认生成
  `sunflower-buddy.bin` 并完成最终分区大小检查。
- 启动日志应显示项目 `sunflower-buddy`、预期的 Git 派生 App 版本、
  ESP-IDF 5.5.3、16 MB Flash 和 8 MB Octal PSRAM。
- 上游构建命令产生的 `esptool` deprecated option 警告，在镜像生成和大小检查成功时
  不属于编译失败。
- 启动时的 `Incorrect size of core dump image` 通常表示 coredump 分区含有旧数据或
  非 coredump 内容，不代表本次启动发生崩溃；确认旧内容不再需要后，只擦除该专用分区。

## 结果报告

以下结果必须分开报告：

1. **Build：** ESP-IDF 版本、命令、target 和结果。
2. **Host tests：** 命令、测试数量和结果。
3. **Device tests：** 板卡版本、固件 commit、观察行为和日志。
4. **Unverified：** 所有未测试的相关物理行为。

Wi-Fi/BLUFI 修改还应记录手机 App 版本，并按 [BLUFI 指南](blufi-provisioning.zh_CN.md)
分别报告清空/已保存 Station 配置、已保存配置启动、断线重连、长按重置和重复会话结果。

真机验证请使用专用开发设备，并按
[Sunflower Buddy 硬件指南](../../hardware-design/SUNFLOWER_BUDDY_V14_HARDWARE_GUIDE.zh_CN.md)检查相关项目。
