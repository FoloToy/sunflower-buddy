<p align="right"><strong>English</strong> · <a href="environment-setup.zh_CN.md">简体中文</a></p>

# Environment Setup

The project requires ESP-IDF 5.5.3 and targets ESP32-S3 only. Do not reuse a
build directory configured by another ESP-IDF Python environment or ESP-IDF
patch release.

## Prerequisites

Install the host packages required by ESP-IDF for your operating system by
following Espressif's official setup guide. Install ESP-IDF outside this
repository, then install the ESP32-S3 tools:

```bash
git clone --branch v5.5.3 --recursive \
  https://github.com/espressif/esp-idf.git "$HOME/esp/esp-idf-v5.5.3"
"$HOME/esp/esp-idf-v5.5.3/install.sh" esp32s3
source "$HOME/esp/esp-idf-v5.5.3/export.sh"
idf.py --version
```

The version output must identify ESP-IDF 5.5.3. Installing packages, changing
USB permissions, or modifying shell startup files are machine-level actions and
should be done explicitly by the developer.

## Initialize a checkout

```bash
git status --short --branch
source /path/to/esp-idf-v5.5.3/export.sh
./tools/validate.sh --static
idf.py set-target esp32s3
idf.py build
```

`idf.py set-target` can replace generated `sdkconfig` state. Preserve intentional
local configuration before running it in an existing checkout, and do not run
it for every incremental build. The generated `sdkconfig`, `build/`, and
`managed_components/` directories are ignored. The committed
`sdkconfig.defaults`, component manifests, and `dependencies.lock` are the
reproducible baseline; never edit `managed_components/` directly.

The first configure may access the ESP Component Registry and the FoloToy driver
Git repository. Later builds use the locked versions in `dependencies.lock`.

## Serial access

Discover the actual port rather than hard-coding one:

```bash
ls /dev/ttyACM* /dev/ttyUSB* 2>/dev/null
idf.py -p <port> monitor
```

On Linux, serial access may require membership in the distribution's serial
device group. Stop other monitors before flashing. Use `Ctrl+]` to leave the
ESP-IDF monitor.

## Common environment failures

| Symptom | Resolution |
| --- | --- |
| `idf.py` not found | Source the selected ESP-IDF `export.sh` in the current shell. |
| CMake says the bootloader source from `v5.5.2` does not match `v5.5.3` | The generated cache came from another IDF tree. Activate 5.5.3, run `idf.py fullclean`, then build again. If `fullclean` cannot parse the stale cache, move only the ignored `build/` directory to a known backup location and reconfigure. |
| Active Python differs from the configured build | Activate one ESP-IDF 5.5.3 environment, remove only generated state with `idf.py fullclean`, then reconfigure. Do not invoke plain `ninja` from a build created by another environment. |
| CMake reports the wrong target | Preserve local configuration, then run `idf.py set-target esp32s3`. |
| Managed component resolution changes | Confirm ESP-IDF 5.5.3, review the manifest and lock-file diff together. |
| Dependency download fails | Confirm network/Git access, then rerun configuration; do not copy a partially generated `managed_components/` into Git. |
