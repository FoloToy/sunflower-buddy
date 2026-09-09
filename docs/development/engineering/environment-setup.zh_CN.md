<p align="right"><a href="environment-setup.md">English</a> · <strong>简体中文</strong></p>

# 环境搭建

本工程只支持 ESP-IDF 5.5.3 和 ESP32-S3。不要复用由另一套 ESP-IDF Python 环境
或其他 ESP-IDF patch 版本配置过的构建目录。

## 前置环境

按照乐鑫官方安装指南为当前操作系统安装主机依赖。ESP-IDF 应安装在仓库外，并安装
ESP32-S3 工具链：

```bash
git clone --branch v5.5.3 --recursive \
  https://github.com/espressif/esp-idf.git "$HOME/esp/esp-idf-v5.5.3"
"$HOME/esp/esp-idf-v5.5.3/install.sh" esp32s3
source "$HOME/esp/esp-idf-v5.5.3/export.sh"
idf.py --version
```

版本输出必须为 ESP-IDF 5.5.3。安装系统包、修改 USB 权限或 shell 启动文件属于
主机级操作，应由开发者明确执行。

## 初始化 checkout

```bash
git status --short --branch
source /path/to/esp-idf-v5.5.3/export.sh
./tools/validate.sh --static
idf.py set-target esp32s3
idf.py build
```

`idf.py set-target` 可能替换生成的 `sdkconfig` 状态。在已有 checkout 中运行前先保留
有意的本地配置，不要在每次增量构建时重复执行。生成的 `sdkconfig`、`build/` 和
`managed_components/` 均已忽略；已提交的 `sdkconfig.defaults`、组件 manifest 和
`dependencies.lock` 是可复现基线，禁止直接修改 `managed_components/`。

首次配置可能访问 ESP Component Registry 和 FoloToy Driver Git 仓库，后续构建使用
`dependencies.lock` 锁定的版本。

## 串口访问

发现实际端口，不要在工程中硬编码：

```bash
ls /dev/ttyACM* /dev/ttyUSB* 2>/dev/null
idf.py -p <port> monitor
```

Linux 可能要求用户属于串口设备组。烧录前关闭其他串口监视器，使用 `Ctrl+]` 退出
ESP-IDF monitor。

## 常见环境错误

| 现象 | 处理 |
| --- | --- |
| 找不到 `idf.py` | 在当前终端加载选定 ESP-IDF 的 `export.sh`。 |
| CMake 提示 bootloader 源码从 `v5.5.2` 变成 `v5.5.3` 且不匹配 | 生成缓存来自另一 IDF 目录。激活 5.5.3，运行 `idf.py fullclean` 后重建；若旧缓存导致 `fullclean` 也无法解析，只把已忽略的 `build/` 移到明确的备份位置，再重新配置。 |
| 当前 Python 与 build 配置不一致 | 激活唯一的 ESP-IDF 5.5.3 环境，用 `idf.py fullclean` 只清理生成状态，再重新配置；不要在另一环境生成的目录中直接运行 `ninja`。 |
| CMake 报告 target 错误 | 保留本地配置后运行 `idf.py set-target esp32s3`。 |
| Managed Component 解析发生变化 | 确认 ESP-IDF 5.5.3，并同时 review manifest 与锁文件差异。 |
| 依赖下载失败 | 检查网络与 Git 访问后重新配置，禁止把未完成的 `managed_components/` 提交进 Git。 |
