[简体中文](README.zh_CN.md)

# Sunflower Buddy

Public hardware BSP for AI agents, following [AI Passport](https://github.com/FoloToy/ai-passport): stable hardware APIs, one pin source of truth, routed context and separate validation evidence.

The default `main/main.cpp` initializes only buses, power and LED. It starts no application services. The standalone `examples/hardware-demo` is built independently and is excluded from the baseline image.

## Start development

Read [AGENTS.md](AGENTS.md), then the task-specific [hardware and API guide](docs/BSP.md). Confirm the physical board revision before changing hardware.

```text
components/bsp/include/   public hardware API; bsp_pins.h is authoritative
components/bsp/src/       reusable board adaptation
main/                     minimal baseline
examples/hardware-demo/   independent diagnostic application
docs/                     hardware, ownership and validation context
tools/build.sh            builds both firmware projects
```

## Build

```sh
. /path/to/esp-idf-v5.5.3/export.sh
./tools/build.sh
# Or build one project:
idf.py build
idf.py -C examples/hardware-demo build
```

Use ESP-IDF 5.5.3, target `esp32s3`. Dependencies are public and the hardware driver is commit-pinned. The baseline has only NVS, PHY and one factory app; demos may have their own layout. A development layout is not a production update contract.

## Scope and validation

No MQTT, TTS, OTA, cloud credentials or product state machine belongs in this BSP. See the hardware guide for initialization, callbacks, blocking, revisions and device acceptance. Report compiler, host tests and device tests separately. FoloToy source is MIT licensed; third-party dependencies retain their notices.
